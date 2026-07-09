@tool
extends Node2D
class_name Tool2D



@export_category("References")
@export var sprite: Sprite2D
@export var tilemap: TileMapLayer

@export_category("Resources")
#@export var generator_settings: DiamondSquareSettings
@export var generator_settings: PerlinNoiseSettings
#@export var erosion_settings: HydraulicErosionSettings

@export_category("Generation Settings")
@export_subgroup("Render Settings")
@export var tile_size: int = 2
@export var use_grayscale_sprite: bool = true
@export var use_grayscale_tilemap: bool = false
@export_subgroup("What to generate")
@export var gen_sprite: bool = true
@export var gen_tilemap: bool = false
@export_subgroup("Smooth")
@export var use_smooth: bool = false
@export var smooth_iterations: int = 5
#@export_subgroup("Erosion")
#@export var erosion_iterations: int = 70000
@export_subgroup("Regenerate")
@export var regenerate: bool = false : set = set_regenerate
#@export var erode: bool = false : set = set_erode

var heightmap: Heightmap = null
var renderer: TerrainRenderer2D
#var generator: DiamondSquare
var generator: PerlinNoise 
#var erosion: HydraulicErosion



func _ready() -> void:
	if !Engine.is_editor_hint():
		generate_terrain()

func set_regenerate(value: bool) -> void:
	if value:
		print("Regenerating terrain...")
		generate_terrain()
		regenerate = false

#func set_erode(value: bool) -> void:
	#if value:
		#print("Eroding terrain...")
		#erode_terrain()
		#erode = false
		#print("Erosion done")



#func erode_terrain() -> void:
	#if !heightmap:
		#print("No heightmap — generate terrain first!")
		#return
	#if !erosion_settings:
		#print("No erosion settings!")
		#return
	#if !sprite: 
		#print("No Sprite2D!")
		#return
	#if !tilemap:
		#print("No TileMapLayer!")
		#return
	#if !renderer:   
		#renderer = TerrainRenderer2D.new()
	#if !erosion:
		#erosion = HydraulicErosion.new()
	#renderer.tile_size = tile_size
	#
	#erosion.erode(heightmap, erosion_iterations, erosion_settings)
	#
	#if gen_sprite:
		#if !sprite: 
			#print("No Sprite2D!")
			#return
		#
		#sprite.texture = renderer.generate_texture_from_config(heightmap)
		#
		#if Engine.is_editor_hint():
			#sprite.queue_redraw()
			#notify_property_list_changed()
		#
		#print("Texture generated!")
	#
	#if gen_tilemap:
		#var tileset = renderer.create_tileset(renderer.colors, renderer.thresholds, renderer.tile_size)
		#tilemap.tile_set = tileset
		#renderer.generate_tilemap_from_config(heightmap, tilemap)
		#print("Tilemap generated!")

func generate_terrain() -> void:
	if !generator_settings: 
		print("No generator settings assigned!")
		return
	if !sprite: 
		print("No Sprite2D!")
		return
	if !tilemap:
		print("No TileMapLayer!")
		return
	if !renderer:
		renderer = TerrainRenderer2D.new()
	if !generator:
		#generator = DiamondSquare.new()
		generator = PerlinNoise.new()
	renderer.tile_size = tile_size
	
	print("Generating terrain...")
	
	heightmap = generator.generate(generator_settings)
	print("Heightmap size: ", heightmap.get_size())
	
	if use_smooth: 
		heightmap.smooth(smooth_iterations)
	
	if gen_sprite:
		if !sprite: 
			print("No Sprite2D!")
			return
		
		sprite.texture = renderer.generate_texture_from_config(heightmap, use_grayscale_sprite)
		
		if Engine.is_editor_hint():
			sprite.queue_redraw()
			notify_property_list_changed()
		
		print("Texture generated!")
	
	if gen_tilemap:
		var tileset = renderer.create_tileset(renderer.colors, renderer.thresholds, renderer.tile_size)
		tilemap.tile_set = tileset
		renderer.generate_tilemap_from_config(heightmap, tilemap, use_grayscale_tilemap)
		print("Tilemap generated!")
	
	print("Complete!")
