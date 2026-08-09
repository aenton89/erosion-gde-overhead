# uruchamia algorytmy erozji na zadanych rozmiarach map, zapisuje wyniki do osobnych plików CSV przez Profiler
@tool
extends Node
class_name Benchmark



@export_category("Algorithm Settings")
@export var perlin_settings: PerlinNoiseSettings
@export var pipe_settings: PipeErosionSettings
@export var hydraulic_settings: HydraulicErosionSettings

# (map_size, pipe_iterations, hydraulic_iterations)
@export_category("Map Sizes")
@export var map_configs: Array[Vector3i] = [
	Vector3i(32, 2, 1250),
	Vector3i(64, 8, 5000),
	Vector3i(128, 32, 20000),
	Vector3i(256, 128, 80000),
	Vector3i(512, 512, 320000),
]

@export_category("Run Settings")
@export var repeats: int = 10
@export var output_dir: String = "../data_analysis/data/"
@export_subgroup("Threads")
@export var auto_thread_counts: bool = true
@export var thread_counts: Array[int] = [1, 2, 4, 6, 8, 10, 12]
@export var memory_thread_count: int = 6
@export_subgroup("Time")
@export var measure_timing: bool = true
@export_subgroup("Memory")
@export var measure_memory: bool = false
@export var memory_sample_interval_ms: int = 2
@export_subgroup("Overhead Measures")
@export var overhead_n: int = 1000000
@export var overhead_arr_sizes: Array = [32*32, 64*64, 128*128, 256*256, 512*512]
@export_subgroup("Divergence")
@export var divergence_map_size: int = 128
@export var divergence_iterations: Array[int] = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512]

@export_category("Run")
@export_subgroup("Main Settings")
@export var auto_run_on_launch: bool = true
@export var generate_fixtrs: bool = false : set = on_generate_fixtrs
@export var run_full: bool = false : set = on_run_full
@export var run_benchmarks: bool = false : set = on_run_benchmarks
@export_subgroup("Single Benchmarks")
@export var run_pipe_optimized: bool = false : set = on_run_pipe_optimized
@export var run_pipe_cpp: bool = false : set = on_run_pipe_cpp
@export var run_pipe_gds: bool = false : set = on_run_pipe_gds
@export var run_hydraulic_cpp: bool = false : set = on_run_hydraulic_cpp
@export var run_hydraulic_gds: bool = false : set = on_run_hydraulic_gds
@export var run_overhead: bool = false : set = on_run_overhead
@export var run_correctness: bool = false : set = on_run_correctness
@export var run_divergence: bool = false : set = on_run_divergence


var overhead_msr: OverheadMeasure
var pipe_settings_gds: PipeErosionSettingsGDS
var hydraulic_settings_gds: HydraulicErosionSettingsGDS



func _ready() -> void:
	if Engine.is_editor_hint():
		return
	if !auto_run_on_launch:
		return
	
	await run_both_benchmarks()



func on_run_full(v: bool) -> void:
	if v:
		await run_both_benchmarks()
		run_full = false

func on_run_benchmarks(v: bool) -> void:
	if v:
		run_benchmark()
		run_benchmarks = false

func on_run_pipe_optimized(v: bool) -> void:
	if v:
		run_single("pipe_optimized")
		run_pipe_optimized = false

func on_run_pipe_cpp(v: bool) -> void:
	if v:
		run_single("pipe_cpp")
		run_pipe_cpp = false

func on_run_pipe_gds(v: bool) -> void:
	if v:
		run_single("pipe_gds")
		run_pipe_gds = false

func on_run_hydraulic_cpp(v: bool) -> void:
	if v:
		run_single("hydraulic_cpp")
		run_hydraulic_cpp = false

func on_run_hydraulic_gds(v: bool) -> void:
	if v:
		run_single("hydraulic_gds")
		run_hydraulic_gds = false

func on_run_overhead(v: bool) -> void:
	if v:
		run_overhead_benchmark(512)
		run_overhead = false

func on_generate_fixtrs(v: bool) -> void:
	if v:
		generate_terrain_fixtrs()
		generate_fixtrs = false

func on_run_correctness(v: bool) -> void:
	if v:
		run_correctness_check()
		run_correctness = false

func on_run_divergence(v: bool) -> void:
	if v:
		run_divergence_check()
		run_divergence = false



func run_both_benchmarks() -> void:
	run_benchmark()
	measure_memory = !measure_memory
	measure_timing = !measure_timing
	
	run_benchmark()
	measure_memory = !measure_memory
	measure_timing = !measure_timing
	
	print("BOTH BENCHMARKS DONE 'N SAVED")
	if Engine.is_editor_hint():
		return
	await get_tree().process_frame
	get_tree().quit()

func run_benchmark() -> void:
	if !validate(): 
		return
	
	print("=" .repeat(60))
	print("TERRAIN BENCHMARK START")
	Engine.get_singleton("Profiler").print_environment()
	print("=" .repeat(60))
	
	run_single("pipe_optimized")
	run_single("pipe_cpp")
	run_single("pipe_gds")
	run_single("hydraulic_cpp")
	run_single("hydraulic_gds")
	
	if !measure_memory:
		run_overhead_benchmark(512)
		run_correctness_check()
		run_divergence_check()
	
	print("=" .repeat(60))
	print("BENCHMARK DONE")
	print("=" .repeat(60))

func run_single(algo: String) -> void:
	if !validate(): 
		return
	
	var P: = Engine.get_singleton("Profiler")
	var t_path: String = times_path(algo)
	var m_path: String = memory_sampled_path(algo)
	
	if measure_timing:
		ensure_dir(t_path)
		P.begin_results_file(t_path, "caller,thread_id,function,duration_us,threads_used,map_width,map_height")
	if measure_memory:
		ensure_dir(m_path)
		P.begin_results_file(m_path, "label,run,elapsed_us,map_width,map_height,working_set_bytes,peak_working_set_bytes,private_bytes")
	
	print("-".repeat(50))
	print("Benchmarking: %s (repeats=%d)" % [algo, repeats])
	
	# wątki tylko dla zoptymalizowanej wersji
	var threads_to_test: Array 
	if algo == "pipe_optimized" and measure_timing:
		threads_to_test = resolve_thread_counts()
	elif algo == "pipe_optimized":
		threads_to_test = [resolve_memory_thread_count()]
	else:
		threads_to_test = [1]
	
	for cfg in map_configs:
		var map_size: int = cfg.x
		var pipe_iters: int = cfg.y
		var hydraulic_iters: int = cfg.z
		var iters: int = pipe_iters if is_pipe(algo) else hydraulic_iters
		
		print(" map=%dx%d iterations=%d" % [map_size, map_size, iters])
		
		if measure_timing or measure_memory:
			for threads in threads_to_test:
				P.set_thread_count(threads)
				
				for rnb in range(repeats):
					P.set_map_size(map_size, map_size)
					run_algo(algo, map_size, iters, P, rnb)
				
				if measure_timing:
					P.save_time_csv(t_path)
					print(" -> saved: %s" % t_path)
				if measure_memory:
					P.save_memory_csv(m_path)
					print(" -> saved: %s" % m_path)
	
	print(" DONE: %s" % algo)



func generate_heightmap(map_size: int, algo_label: String) -> Object:
	# konfiguruj generator z aktualnym rozmiarem mapy
	var gen
	if algo_label.ends_with("gds"):
		gen = PerlinNoiseGeneratorGDS.new()
	else:
		gen = PerlinNoise.new()
	
	# tymczasowo nadpisz map_size w settings
	var orig_size: int = perlin_settings.map_size
	perlin_settings.map_size = map_size
	
	var heightmap = gen.generate(perlin_settings)
	#var heightmap = P.profile(algo_label, "generate", Callable(gen, "generate"), [perlin_settings])
	
	perlin_settings.map_size = orig_size
	return heightmap

func run_algo(algo: String, map_size: int, iters: int, P: Object, repeat_nb: int) -> void:
	var heightmap: = load_heightmap_for(algo, map_size)
	
	match algo:
		"pipe_optimized":
			var erosion: PipeErosionOptimized = PipeErosionOptimized.new()
			call_erode(erosion, algo, heightmap, iters, pipe_settings, P, repeat_nb)
		"pipe_cpp":
			var erosion: PipeErosion = PipeErosion.new()
			call_erode(erosion, algo, heightmap, iters, pipe_settings, P, repeat_nb)
		"pipe_gds":
			var erosion: PipeErosionGDS = PipeErosionGDS.new()
			pipe_settings_gds = PipeErosionSettingsGDS.new()
			pipe_settings_gds.copy_from_cpp(pipe_settings)
			call_erode(erosion, algo, heightmap, iters, pipe_settings_gds, P, repeat_nb)
		"hydraulic_cpp":
			var erosion: HydraulicErosion = HydraulicErosion.new()
			call_erode(erosion, algo, heightmap, iters, hydraulic_settings, P, repeat_nb)
		"hydraulic_gds":
			var erosion: HydraulicErosionGDS = HydraulicErosionGDS.new()
			hydraulic_settings_gds = HydraulicErosionSettingsGDS.new()
			hydraulic_settings_gds.copy_from_cpp(hydraulic_settings)
			call_erode(erosion, algo, heightmap, iters, hydraulic_settings_gds, P, repeat_nb)

func call_erode(erosion: Object, algo: String, heightmap: Object, iters: int, settings: Resource, P: Object, repeat_nb: int) -> void:
	if measure_memory:
		P.start_memory_sampling(memory_sample_interval_ms, algo, repeat_nb)
	if measure_timing:
		P.profile(algo, "erode", Callable(erosion, "erode"), [heightmap, iters, settings])
	else:
		erosion.erode(heightmap, iters, settings)
	if measure_memory:
		P.stop_memory_sampling()

func run_overhead_benchmark(map_size: int) -> void:
	if !validate():
		return
	
	var P: = Engine.get_singleton("Profiler")
	var path: String = overhead_path("overhead")
	ensure_dir(path)
	
	print("-".repeat(50))
	print("Benchmarking: overhead (repeats=%d, N=%d)" % [repeats, overhead_n])
	
	var heightmap: = generate_heightmap(map_size, "overhead")
	
	P.clear_time()
	
	if !overhead_msr:
		overhead_msr = OverheadMeasure.new()
	
	overhead_msr.measure(P, repeats, overhead_n, overhead_arr_sizes, path, pipe_settings, heightmap)

func run_correctness_check() -> void:
	if !validate():
		return
	
	print("-".repeat(50))
	print("Correctness check: pipe_cpp vs pipe_optimized vs pipe_gds")
	
	for cfg in map_configs:
		var map_size: int = cfg.x
		var iters: int = cfg.y
		
		var hm_base: Heightmap = load_heightmap_for("pipe_cpp", map_size)
		var base: PipeErosion = PipeErosion.new()
		base.erode(hm_base, iters, pipe_settings)
		Helpers.save_terrain_binary(correctness_path("pipe_baseline", map_size), hm_base.get_size(), hm_base.get_data())
		
		var hm_opt: Heightmap = load_heightmap_for("pipe_optimized", map_size)
		var opt: PipeErosionOptimized = PipeErosionOptimized.new()
		opt.erode(hm_opt, iters, pipe_settings)
		Helpers.save_terrain_binary(correctness_path("pipe_optimized", map_size), hm_opt.get_size(), hm_opt.get_data())
		
		var hm_gds: HeightmapGDS = load_heightmap_for("pipe_gds", map_size)
		var gds: PipeErosionGDS = PipeErosionGDS.new()
		pipe_settings_gds = PipeErosionSettingsGDS.new()
		pipe_settings_gds.copy_from_cpp(pipe_settings)
		gds.erode(hm_gds, iters, pipe_settings_gds)
		Helpers.save_terrain_binary(correctness_path("pipe_gds", map_size), hm_gds.get_size(), hm_gds.get_data())
		
		print(" map=%dx%d done" % [map_size, map_size])

func run_divergence_check() -> void:
	if !validate():
		return
	
	print("-".repeat(50))
	print("Divergence check: map=%dx%d" % [divergence_map_size, divergence_map_size])
	
	for iters in divergence_iterations:
		var hm_base: Heightmap = load_heightmap_for("pipe_cpp", divergence_map_size)
		var base: PipeErosion = PipeErosion.new()
		base.erode(hm_base, iters, pipe_settings)
		Helpers.save_terrain_binary(divergence_path("baseline", iters), hm_base.get_size(), hm_base.get_data())
		
		var hm_opt: Heightmap = load_heightmap_for("pipe_optimized", divergence_map_size)
		var opt: PipeErosionOptimized = PipeErosionOptimized.new()
		opt.erode(hm_opt, iters, pipe_settings)
		Helpers.save_terrain_binary(divergence_path("optimized", iters), hm_opt.get_size(), hm_opt.get_data())
		
		var hm_gds: HeightmapGDS = load_heightmap_for("pipe_gds", divergence_map_size)
		var gds: PipeErosionGDS = PipeErosionGDS.new()
		pipe_settings_gds = PipeErosionSettingsGDS.new()
		pipe_settings_gds.copy_from_cpp(pipe_settings)
		gds.erode(hm_gds, iters, pipe_settings_gds)
		Helpers.save_terrain_binary(divergence_path("gds", iters), hm_gds.get_size(), hm_gds.get_data())
		
		print(" iters=%d done" % iters)

func generate_terrain_fixtrs() -> void:
	ensure_dir(terrain_fixtrs_path(0))
	
	for cfg in map_configs:
		var map_size: int = cfg.x
		var path: String = terrain_fixtrs_path(map_size)
		
		var orig_size: int = perlin_settings.map_size
		perlin_settings.map_size = map_size
		var gen: PerlinNoise = PerlinNoise.new()
		var hm: Heightmap = gen.generate(perlin_settings)
		perlin_settings.map_size = orig_size
		
		Helpers.save_terrain_binary(path, hm.get_size(), hm.get_data())
		print("saved fixture: %s" % path)

func load_heightmap_for(algo: String, map_size: int) -> Object:
	var path: String = terrain_fixtrs_path(map_size)
	var size: int = Helpers.load_terrain_size(path)
	var data: PackedFloat32Array = Helpers.load_terrain_data(path)
	
	if algo.ends_with("gds"):
		var hm: HeightmapGDS = HeightmapGDS.new(size)
		hm.data = data
		return hm
	
	var hm: Heightmap = Heightmap.new()
	hm.load_from_data(data, size)
	return hm



func is_pipe(algo: String) -> bool:
	return algo.begins_with("pipe")

# liczby wątków do przebiegu skalowania - z profilera albo z ustawienia ręcznego
func resolve_thread_counts() -> Array:
	if !auto_thread_counts:
		return thread_counts
	var P: = Engine.get_singleton("Profiler")
	if !P:
		return thread_counts
	var auto_counts: PackedInt32Array = P.get_auto_thread_counts()
	if auto_counts.is_empty():
		return thread_counts
	return Array(auto_counts)

# pojedyncza liczba wątków dla pomiarów pamięci - domyślnie rdzenie fizyczne
func resolve_memory_thread_count() -> int:
	if !auto_thread_counts:
		return memory_thread_count
	var P: = Engine.get_singleton("Profiler")
	if !P:
		return memory_thread_count
	var phys: int = P.get_physical_core_count()
	return phys if phys > 0 else P.get_auto_thread_count()



func times_path(algo: String) -> String:
	var base: String = ProjectSettings.globalize_path("res://") + output_dir + "times/"
	return base + "%s_times.csv" % algo

func overhead_path(algo: String) -> String:
	var base: String = ProjectSettings.globalize_path("res://") + output_dir + "overhead/"
	return base + "%s_profile.csv" % algo

func memory_sampled_path(algo: String) -> String:
	var base: String = ProjectSettings.globalize_path("res://") + output_dir + "memory/"
	return base + "%s_memory_sampled.csv" % algo

func terrain_fixtrs_path(map_size: int) -> String:
	var base: String = ProjectSettings.globalize_path("res://") + output_dir + "fixtures/"
	return base + "terrain_fixture_%d.bin" % map_size

func correctness_path(label: String, map_size: int) -> String:
	var base: String = ProjectSettings.globalize_path("res://") + output_dir + "fixtures/"
	return base + "%s_terrain_%d.bin" % [label, map_size]

func divergence_path(label: String, iters: int) -> String:
	var base: String = ProjectSettings.globalize_path("res://") + output_dir + "fixtures/"
	return base + "div_%s_%d.bin" % [label, iters]

func ensure_dir(path: String) -> void:
	var dir: String = path.get_base_dir()
	DirAccess.make_dir_recursive_absolute(dir)

func validate() -> bool:
	if !Engine.get_singleton("Profiler"):
		push_error("Profiler singleton not found!")
		return false
	if !perlin_settings:
		push_error("perlin_settings not assigned!")
		return false
	if !pipe_settings:
		push_error("pipe_settings not assigned!")
		return false
	if !hydraulic_settings:
		push_error("hydraulic_settings not assigned!")
		return false
	if map_configs.is_empty():
		push_error("map_configs is empty!")
		return false
	for cfg in map_configs:
		var fpath: String = terrain_fixtrs_path(cfg.x)
		if !FileAccess.file_exists(fpath):
			push_error("Benchmark: missing fixture %s - first run generate_terrain_fixtrs" % fpath)
			return false
	return true
