@tool
extends Node3D
class_name ToolPipeGDS



@export_category("References")
@export var mesh_instance: MeshInstance3D

@export_category("Resources")
@export var generator_settings: PerlinNoiseSettings
@export var erosion_settings: PipeErosionSettings

@export_category("Generation Settings")
@export_subgroup("Render Settings")
@export var height_scale: float = 1.0
@export var cell_size: float = 1.0
@export_subgroup("Smooth")
@export var use_smooth: bool = false
@export var smooth_iterations: int = 5
@export_subgroup("Erosion")
@export var erosion_iterations: int = 70000
@export_subgroup("Regenerate")
@export var regenerate: bool = false : set = set_regenerate
@export var erode: bool = false : set = set_erode

@export_category("Shader")
@export var use_height_color: bool = false : set = set_use_height_color
@export var use_improved_steepness: bool = false : set = set_use_improved_steepness
@export var shader_mat: ShaderMaterial

var heightmap: HeightmapGDS = null

var erosion: PipeErosionGDS
var generator: PerlinNoiseGeneratorGDS
var renderer: TerrainMeshBuilderGDS



func _ready() -> void:	
	if !Engine.is_editor_hint():
		generate_terrain()

func set_regenerate(value: bool) -> void:
	if value:
		print("Regenerating terrain...")
		generate_terrain()
		regenerate = false
		print("Terrain regenerated")

func set_erode(value: bool) -> void:
	if value:
		print("Eroding terrain...")
		erode_terrain()
		erode = false
		print("Erosion done")

func set_use_height_color(value: bool) -> void:
	use_height_color = value
	apply_material()

func set_use_improved_steepness(value: bool) -> void:
	use_improved_steepness = value
	apply_material()



func erode_terrain() -> void:
	if !mesh_instance:
		push_error("No MeshInstance3D child!")
		return
	if !heightmap:
		print("No heightmap - generate terrain first!")
		return
	if !erosion:
		erosion= PipeErosionGDS.new()
	if !renderer:
		renderer = TerrainMeshBuilderGDS.new()
	
	var P: = Engine.get_singleton("Profiler")
	
	P.profile("PipeGDS", "erode", Callable(erosion, "erode"), [heightmap, erosion_iterations, erosion_settings])
	
	mesh_instance.mesh = P.profile("PipeGDS", "generate_mesh", Callable(renderer, "generate_mesh"), [heightmap])
	apply_material()
	
	var data_dir: String = ProjectSettings.globalize_path("res://") + "../data_analysis/data/"
	DirAccess.make_dir_recursive_absolute(data_dir)
	P.save_csv(data_dir + "pipe_gds.csv")

func generate_terrain() -> void:
	if !mesh_instance:
		push_error("No MeshInstance3D child!")
		return
	if !generator:
		generator = PerlinNoiseGeneratorGDS.new()
	if !renderer:
		renderer = TerrainMeshBuilderGDS.new()
	
	var P: = Engine.get_singleton("Profiler")
	P.set_map_size(generator_settings.map_size, generator_settings.map_size)
	
	heightmap = P.profile("PerlinNoise3D", "generate", Callable(generator, "generate"), [generator_settings])
	
	if use_smooth:
		heightmap.smooth(smooth_iterations)
	
	renderer.height_scale = height_scale
	renderer.cell_size = cell_size
	
	mesh_instance.mesh = P.profile("PerlinNoise3D", "generate_mesh", Callable(renderer, "generate_mesh"), [heightmap])
	apply_material()
	
	var data_dir: String = ProjectSettings.globalize_path("res://") + "../data_analysis/data/"
	DirAccess.make_dir_recursive_absolute(data_dir)
	P.save_csv(data_dir + "pipe_gds.csv")

func apply_material() -> void:
	if !mesh_instance or !mesh_instance.mesh:
		return
	if shader_mat == null:
		shader_mat = ShaderMaterial.new()
		shader_mat.shader = load("res://assets/terrain.gdshader")
	
	shader_mat.set_shader_parameter("use_height_color", use_height_color)
	mesh_instance.material_override = shader_mat
