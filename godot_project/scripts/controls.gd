extends Node
class_name BasicControls



@export_category("References")
@export var parent: Node
@export var camera2d: Camera2D
@export var camera3d: Camera3D
@export_category("Camera 2D Movement")
@export var speed: float = 400.0
@export var zoom_step: float = 0.1
@export var min_zoom: float = 0.1
@export var max_zoom: float = 8.0
@export_category("Camera 3D Movement")
@export var base_speed: float = 5.0
@export var speed_multiplier: float = 1.1
@export var mouse_sensitivity: float = 0.002
@export var fast_multiplier: float = 4.0

var current_speed: float = 5.0
var yaw: float = 0.0
var pitch: float = 0.0
var rotating: bool = false
var dragging: bool = false



func _process(delta):
	if Input.is_action_just_pressed("exit"):
		get_tree().quit()
	
	if Input.is_action_just_pressed("regenerate"):
		parent.generate_terrain()
	
	if camera2d:
		var dir: Vector2 = Vector2.ZERO
		if Input.is_action_pressed("up"):
			dir.y -= 1
		if Input.is_action_pressed("down"):
			dir.y += 1
		if Input.is_action_pressed("left"):
			dir.x -= 1
		if Input.is_action_pressed("right"):
			dir.x += 1
		
		if dir != Vector2.ZERO:
			print(camera2d.zoom.y)
			camera2d.position += dir.normalized() * speed * delta / camera2d.zoom
	
	elif camera3d:
		if rotating:
			var dir: Vector3 = Vector3.ZERO
			
			if Input.is_action_pressed("up"):
				dir.z -= 1
			if Input.is_action_pressed("down"):
				dir.z += 1
			if Input.is_action_pressed("left"):
				dir.x -= 1
			if Input.is_action_pressed("right"):
				dir.x += 1
			
			if dir != Vector3.ZERO:
				var speed_temp: float = current_speed
				
				if Input.is_action_pressed("move_faster"):
					speed_temp *= fast_multiplier
				
				var basis: Basis = camera3d.global_transform.basis
				var move: Vector3 = (basis.x * dir.x + basis.y * dir.y + basis.z * dir.z).normalized()
				
				camera3d.global_position += move * speed_temp * delta

func _unhandled_input(event):
	if camera2d:
		if event is InputEventMouseButton:
			if event.button_index == MOUSE_BUTTON_WHEEL_DOWN and event.pressed:
				camera2d.zoom = (camera2d.zoom - Vector2(zoom_step, zoom_step)).clamp(
					Vector2(min_zoom, min_zoom), Vector2(max_zoom, max_zoom))
			elif event.button_index == MOUSE_BUTTON_WHEEL_UP and event.pressed:
				camera2d.zoom = (camera2d.zoom + Vector2(zoom_step, zoom_step)).clamp(
					Vector2(min_zoom, min_zoom), Vector2(max_zoom, max_zoom))
		
			elif event.button_index == MOUSE_BUTTON_LEFT:
				dragging = event.pressed
		
		elif event is InputEventMouseMotion and dragging:
			camera2d.position -= event.relative / camera2d.zoom
	
	elif camera3d:
		if event is InputEventMouseButton:
			if event.button_index == MOUSE_BUTTON_RIGHT:
				rotating = event.pressed
				Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED if rotating else Input.MOUSE_MODE_VISIBLE)
			
			# scroll = zmiana prędkości (jak w Unity)
			if event.pressed:
				if event.button_index == MOUSE_BUTTON_WHEEL_UP:
					current_speed *= speed_multiplier
				elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
					current_speed /= speed_multiplier
		
		elif event is InputEventMouseMotion and rotating:
			yaw -= event.relative.x * mouse_sensitivity
			pitch -= event.relative.y * mouse_sensitivity
			pitch = clamp(pitch, -1.5, 1.5)
			
			camera3d.rotation = Vector3(pitch, yaw, 0)
