extends Resource
class_name PipeErosionSettingsGDS



@export_group("Simulation")
@export var dt: float = 0.02
@export var pipe_length: float = 1.0
@export var pipe_area: float = 0.1
@export var gravity: float = 9.81
@export_group("Sediment")
@export var Kc: float = 1.0
@export var Ks: float = 0.5
@export var Kd: float = 0.5
@export_group("Rain / Evaporation")
@export var Ke: float = 0.01
@export var rain_rate: float = 0.01
@export var rain_iterations: int = 1
@export_group("Stability")
@export var min_tilt_sin: float = 0.05
@export var max_velocity: float = 10.0



func copy_from_cpp(config: PipeErosionSettings) -> void:
	dt = config.dt
	pipe_length = config.pipe_length
	pipe_area = config.pipe_area
	gravity = config.gravity
	Kc = config.Kc
	Ks = config.Ks
	Kd = config.Kd
	Ke = config.Ke
	rain_rate = config.rain_rate
	rain_iterations = config.rain_iterations
	min_tilt_sin = config.min_tilt_sin
	max_velocity = config.max_velocity
