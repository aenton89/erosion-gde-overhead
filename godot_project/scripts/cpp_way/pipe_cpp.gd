@tool
extends Node3D
class_name ToolPipeCPP



@export_category("References")
@export var mesh_instance: MeshInstance3D

@export_category("Resources")
@export var generator_settings: PerlinNoiseSettings
@export_subgroup("Erosion")
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

# przechowuje ostatnią heightmapę żeby erode_terrain() mogła jej użyć
var heightmap: Heightmap = null
var generator: PerlinNoise
var erosion: PipeErosion
var renderer: TerrainRenderer3D



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
	if !heightmap:
		print("No heightmap - generate terrain first!")
		return
	if !erosion_settings:
		print("No erosion settings!")
		return
	if !mesh_instance: 
		print("No MeshInstance3D!")
		return
	if !renderer:   
		renderer = TerrainRenderer3D.new()
	if !erosion:
		erosion = PipeErosion.new()
	renderer.cell_size = cell_size
	renderer.height_scale = height_scale
	
	var P: = Engine.get_singleton("Profiler")
	
	P.profile("PipeCPP", "erode", Callable(erosion, "erode"), [heightmap, erosion_iterations, erosion_settings])
	
	mesh_instance.mesh = P.profile("PipeCPP", "generate_mesh", Callable(renderer, "generate_mesh"), [heightmap])
	apply_material()
	
	var data_dir: String = ProjectSettings.globalize_path("res://") + "../data_analysis/data/"
	DirAccess.make_dir_recursive_absolute(data_dir)
	P.save_csv(data_dir + "pipe_cpp.csv")

func generate_terrain() -> void:
	if !generator_settings:
		print("No generator settings!")
		return
	if !mesh_instance: 
		print("No MeshInstance3D!")
		return
	if !generator:  
		generator = PerlinNoise.new()
	if !renderer:   
		renderer = TerrainRenderer3D.new()
	renderer.cell_size = cell_size
	renderer.height_scale = height_scale
	
	var P: = Engine.get_singleton("Profiler")
	
	P.set_map_size(generator_settings.map_size, generator_settings.map_size)
	
	print("Generating terrain...")
	
	heightmap = P.profile("PerlinNoise3D", "generate", Callable(generator, "generate"), [generator_settings])
	print("Heightmap size: ", heightmap.get_size())
	
	if use_smooth: 
		heightmap.smooth(smooth_iterations)
	
	mesh_instance.mesh = P.profile("PerlinNoise3D", "generate_mesh", Callable(renderer, "generate_mesh"), [heightmap])
	apply_material()
	
	var data_dir: String = ProjectSettings.globalize_path("res://") + "../data_analysis/data/"
	DirAccess.make_dir_recursive_absolute(data_dir)
	var path: String = data_dir + "pipe_cpp.csv"
	P.save_csv(path)
	
	print("Mesh generated!")

func apply_material() -> void:
	if !mesh_instance or !mesh_instance.mesh:
		return
	if shader_mat == null:
		shader_mat = ShaderMaterial.new()
		shader_mat.shader = load("res://assets/terrain.gdshader")
	
	shader_mat.set_shader_parameter("use_height_color", use_height_color)
	mesh_instance.material_override = shader_mat
