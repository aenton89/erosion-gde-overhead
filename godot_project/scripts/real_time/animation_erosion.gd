@tool
extends Node3D
class_name ErosionAnimation



@export var mesh_instance: MeshInstance3D
#@export var generator_settings: PerlinNoiseGeneratorGDS
@export var generator_settings: PerlinNoiseSettings
@export var pipe_settings: PipeErosionSettings
@export var hydraulic_settings: HydraulicErosionSettings

@export_category("Render")
@export var height_scale: float = 1.0
@export var use_height_color: bool = false : set = set_use_height_color
@export var camera: Camera3D

@export_category("Real-time Erosion")
@export var iterations_at_once: int = 5
@export var run_realtime: bool = false
@export var active_impl: String = "pipe_optimized"

@export_category("Benchmark Real-time")
@export var use_fixture: bool = true
@export var fixtrs_dir: String = "../data_analysis/data/fixtures/"
@export var realt_dir: String = "../data_analysis/data/realtime/"
@export var benchmark_duration_sec: float = 10.0
@export var warmup_frames: int = 5
@export var iters_at_once_values: Array[int] = [1, 2, 3, 5, 8, 13, 21, 34]
@export var map_sizes: Array[int] = [32, 64, 128, 256, 512]
@export var auto_run_benchmark_on_launch: bool = true

@export_category("Preview (Editor)")
@export var preview_duration_sec: float = 5.0
@export var run_preview: bool = false : set = on_run_preview

@export_category("Debug")
@export var regenerate_cpp: bool = false : set = on_regenerate_cpp
@export var regenerate_gds: bool = false : set = on_regenerate_gds

const HYD_IMPLS: Array[String] = ["hydraulic_gds", "hydraulic_cpp"]
const PIP_IMPLS: Array[String] = ["pipe_gds", "pipe_cpp", "pipe_optimized"]

var rt_renderer: TerrainRealtimeRenderer = TerrainRealtimeRenderer.new()
# GDS albo CPP
var heightmap
var erosion
var pipe_settings_gds: PipeErosionSettingsGDS
var hydraulic_settings_gds: HydraulicErosionSettingsGDS



func _ready() -> void:
	if Engine.is_editor_hint():
		return
	
	if auto_run_benchmark_on_launch:
		await run_full_benchmark()
		run_realtime = false
	else:
		setup_erosion(active_impl)
		setup_renderer(generator_settings.map_size)
		generate_terrain()

func _process(_delta: float) -> void:
	if !run_realtime or !heightmap or !current_settings():
		return
	
	# N iteracji erozji per klatkę
	erosion.erode(heightmap, iterations_at_once, current_settings())
	# aktualizacja tekstury: O(size*size) memcpy, bez przebudowy mesha (chyba?)
	rt_renderer.update_raw(heightmap_data())



func set_use_height_color(value: bool) -> void:
	use_height_color = value
	if rt_renderer:
		rt_renderer.set_use_height_color(value)

func set_run_realtime(value: bool) -> void:
	run_realtime = value

func on_regenerate_cpp(value: bool) -> void:
	if value:
		var target: String = "pipe_optimized" if is_pipe(active_impl) else "hydraulic_cpp"
		setup_erosion(target)
		if generator_settings:
			setup_renderer(generator_settings.map_size)
		generate_terrain()
		regenerate_cpp = false

func on_regenerate_gds(value: bool) -> void:
	if value:
		var target: String = "pipe_gds" if is_pipe(active_impl) else "hydraulic_gds"
		setup_erosion(target)
		if generator_settings:
			setup_renderer(generator_settings.map_size)
		generate_terrain()
		regenerate_gds = false

func on_run_preview(value: bool) -> void:
	if value:
		run_preview_once()
		run_preview = false



func is_gds(impl_name: String) -> bool:
	return impl_name.ends_with("gds")
 
func is_pipe(impl_name: String) -> bool:
	return impl_name.begins_with("pipe")

func current_settings() -> Resource:
	if is_gds(active_impl):
		return pipe_settings_gds if is_pipe(active_impl) else hydraulic_settings_gds
	else:
		return pipe_settings if is_pipe(active_impl) else hydraulic_settings

func heightmap_data() -> PackedFloat32Array:
	return heightmap.data if is_gds(active_impl) else heightmap.get_data()



func setup_erosion(impl_name: String) -> void:
	active_impl = impl_name
	
	match impl_name:
		"pipe_optimized":
			erosion = PipeErosionOptimized.new()
		"pipe_cpp":
			erosion = PipeErosion.new()
		"pipe_gds":
			erosion = PipeErosionGDS.new()
			pipe_settings_gds = PipeErosionSettingsGDS.new()
			pipe_settings_gds.copy_from_cpp(pipe_settings)
		"hydraulic_cpp":
			erosion = HydraulicErosion.new()
		"hydraulic_gds":
			erosion = HydraulicErosionGDS.new()
			hydraulic_settings_gds = HydraulicErosionSettingsGDS.new()
			hydraulic_settings_gds.copy_from_cpp(hydraulic_settings)
		_:
			push_error("ErosionAnimation: unknown erosion implementation: %s" % impl_name)
	
	if !rt_renderer:
		rt_renderer = TerrainRealtimeRenderer.new()

func setup_renderer(map_size: int) -> void:
	rt_renderer.setup_mesh_and_material(mesh_instance, map_size, height_scale, "res://assets/terrain_realtime.gdshader")
	rt_renderer.set_use_height_color(use_height_color)
	frame_camera(map_size)

func generate_terrain(map_size_override: int = -1) -> void:
	if !generator_settings or !mesh_instance:
		return
	if !erosion:
		setup_erosion(active_impl)
	
	var map_size: int = map_size_override if map_size_override > 0 else generator_settings.map_size
	
	if use_fixture:
		load_fixture(map_size)
	else:
		generate_new_terrain(map_size)
	
	rt_renderer.update_raw(heightmap_data())

func generate_new_terrain(map_size: int) -> void:
	if is_gds(active_impl):
		var gen: PerlinNoiseGeneratorGDS = PerlinNoiseGeneratorGDS.new()
		heightmap = gen.generate(generator_settings)
	else:
		var gen: PerlinNoise = PerlinNoise.new()
		heightmap = gen.generate(generator_settings)

func load_fixture(map_size: int) -> void:
	var path: String = fixtrs_dir + "terrain_fixture_%d.bin" % map_size
	
	if !FileAccess.file_exists(path):
		push_error("ErosionAnimation: missing fixture (can be generated in benchmark.gd)")
		return
	
	var size: int = Helpers.load_terrain_size(path)
	var data: PackedFloat32Array = Helpers.load_terrain_data(path)
	
	if is_gds(active_impl):
		heightmap = HeightmapGDS.new(size)
		heightmap.data = data
	else:
		heightmap = Heightmap.new()
		heightmap.load_from_data(data, size)



func run_full_benchmark() -> void:
	await run_benchmarks(PIP_IMPLS, "realtime_pipe")
	await run_benchmarks(HYD_IMPLS, "realtime_particle")
	print("ALL REAL-TIME BENCHMARKS DONE 'N SAVED")

func run_benchmarks(impls: Array[String], out_name: String, write_csv: bool = true) -> void:
	if !validate():
		return
	
	run_realtime = false
	
	var raw_path: String = realt_dir + "%s.csv" % out_name
	var summary_path: String = realt_dir + "%s_summary.csv" % out_name
	var P: = Engine.get_singleton("Profiler")
	
	if write_csv:
		ensure_dir(raw_path)
		ensure_dir(summary_path)
		P.begin_results_file(raw_path, "impl,map_size,iterations_at_once,frame_idx,frame_ms")
		P.begin_results_file(summary_path, "impl,map_size,iterations_at_once,frame_count,mean_ms,median_ms,p95_ms,iters_per_sec,within_60fps_budget")
	
	# vsync wyłączany przed i włączany po, żeby ten błąd się już nie zdażył
	var prev_vsync: = DisplayServer.window_get_vsync_mode()
	DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
	
	for map_size in map_sizes:
		# przebudowa mesh raz na rozmiar
		setup_renderer(map_size)
		
		for impl_name in impls:
			setup_erosion(impl_name)
			
			for iters in iters_at_once_values:
				# reset przed każdą próbą
				generate_terrain(map_size)
				
				#if !await benchmark_one(impl_name, map_size, iters, benchmark_duration_sec, raw_path, summary_path, write_csv):
					#break
				await benchmark_one(impl_name, map_size, iters, benchmark_duration_sec, raw_path, summary_path, write_csv)
	
	DisplayServer.window_set_vsync_mode(prev_vsync)
	
	if write_csv:
		print("Real-time benchmark done 'n saved: %s / %s" % [raw_path, summary_path])

func benchmark_one(impl_name: String, map_size: int, iters: int, duration_sec: float, raw_path: String, summary_path: String, write_csv: bool) -> bool:
	var frame_idx: int = 0
	var recorded_ms: Array[float] = []
	var t_start: int = Time.get_ticks_usec()
	
	while (Time.get_ticks_usec() - t_start) / 1000000.0 < duration_sec:
		var t0: int = Time.get_ticks_usec()
		
		erosion.erode(heightmap, iters, current_settings())
		rt_renderer.update_raw(heightmap_data())
		
		var frame_ms: float = (Time.get_ticks_usec() - t0) / 1000.0
		
		frame_idx += 1
		if frame_idx > warmup_frames:
			recorded_ms.append(frame_ms)
			
			if write_csv:
				append_row(raw_path, "%s,%d,%d,%d,%f" % [impl_name, map_size, iters, frame_idx - warmup_frames, frame_ms])
		
		await get_tree().process_frame
	
	return report(impl_name, map_size, iters, recorded_ms, write_csv, summary_path)

func run_preview_once() -> void:
	if !validate():
		return
	
	run_realtime = false
	
	setup_erosion(active_impl)
	var map_size: int = generator_settings.map_size
	setup_renderer(map_size)
	generate_terrain(map_size)
	
	await benchmark_one(active_impl, map_size, iterations_at_once, preview_duration_sec, "", "", false)



func report(impl_name: String, map_size: int, iters: int, recorded_ms: Array[float], write_csv: bool, summary_path: String) -> bool:
	if recorded_ms.is_empty():
		print("At: [%s map=%d @ %d iter] missing frames after warmup - shorter warmup or longer run required" % [impl_name, map_size, iters])
		return false
	
	recorded_ms.sort()
	var n: int = recorded_ms.size()
	var median: float = recorded_ms[n/2]
	var p95: float = recorded_ms[int(n * 0.95)]
	var mean: float = 0.0
	
	for t in recorded_ms:
		mean += t
	mean /= n
	
	var iters_per_sec: float = iters * 1000.0 / mean
	var within_budget: bool = p95 <= 16.67
	
	print("Results: [%s map=%d @ %d iter/frame] frames=%d median=%.2fms p95=%.2fms iter/s=%.0f budget60fps=%s" % [impl_name, map_size, iters, n, median, p95, iters_per_sec, "OK" if within_budget else "EXCEEDED"])
	
	if write_csv:
		append_row(summary_path, "%s,%d,%d,%d,%f,%f,%f,%f,%s" % [impl_name, map_size, iters, n, mean, median, p95, iters_per_sec, "true" if within_budget else "false"])
	
	return within_budget

func append_row(path: String, row: String) -> void:
	var file: FileAccess= FileAccess.open(path, FileAccess.READ_WRITE)
	file.seek_end()
	file.store_line(row)
	file.close()
 
func ensure_dir(path: String) -> void:
	var abs: String = ProjectSettings.globalize_path(path)
	DirAccess.make_dir_recursive_absolute(abs.get_base_dir())

func validate() -> bool:
	var ok: bool = true
	
	if !generator_settings:
		push_error("ErosionAnimation: missing generator_settings")
		ok = false
	if !mesh_instance:
		push_error("ErosionAnimation: missing mesh_instance")
		ok = false
	if !pipe_settings:
		push_error("ErosionAnimation: missing pipe_settings")
		ok = false
	if !hydraulic_settings:
		push_error("ErosionAnimation: missing hydraulic_settings")
		ok = false
	if !ResourceLoader.exists("res://assets/terrain_realtime.gdshader"):
		push_error("ErosionAnimation: missing shader file")
		ok = false
	
	return ok

func frame_camera(map_size: int) -> void:
	if !camera:
		return
	
	camera.position.z = map_size * 0.5
