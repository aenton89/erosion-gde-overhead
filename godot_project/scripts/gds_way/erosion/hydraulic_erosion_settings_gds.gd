extends Resource
class_name HydraulicErosionSettingsGDS



@export_group("Erosion Settings")
@export var seed: int = 0
@export var erosion_radius: int = 3
@export var inertia: float = 0.05
@export var sediment_capacity_factor: float = 4.0
@export var min_sediment_capacity: float = 0.01
@export var erode_speed: float = 0.3
@export var deposit_speed: float = 0.3
@export var evaporate_speed: float = 0.01
@export var gravity: float = 4.0
@export var max_droplet_lifetime: int = 30
@export var initial_water_volume: float = 1.0
@export var initial_speed: float = 1.0



func copy_from_cpp(config: HydraulicErosionSettings) -> void:
	seed = config.seed
	erosion_radius = config.erosion_radius
	inertia = config.inertia
	sediment_capacity_factor = config.sediment_capacity_factor
	min_sediment_capacity = config.min_sediment_capacity
	erode_speed = config.erode_speed
	deposit_speed = config.deposit_speed
	evaporate_speed = config.evaporate_speed
	gravity = config.gravity
	max_droplet_lifetime = config.max_droplet_lifetime
	initial_water_volume = config.initial_water_volume
	initial_speed = config.initial_speed
