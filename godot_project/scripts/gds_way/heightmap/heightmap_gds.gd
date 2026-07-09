extends RefCounted
class_name HeightmapGDS



var data: PackedFloat32Array
var size: int



func _init(map_size: int) -> void:
	size = map_size
	data = PackedFloat32Array()
	data.resize(size * size)

func get_size() -> int:
	return size

func get_data() -> PackedFloat32Array:
	return data

# DEPRECATED: bo to w sumie daje narzut nad wbudowane PackedFloat32Array
#func get_value(x: int, y: int) -> float:
	#return data[y * size + x]
#
#func set_value(x: int, y: int, value: float) -> void:
	#data[y * size + x] = value

func smooth(iterations: int = 1) -> void:
	for _i in range(iterations):
		var copy: PackedFloat32Array = data.duplicate()
		for y in range(size):
			for x in range(size):
				var x0: int = max(x - 1, 0)
				var x1: int = min(x + 1, size - 1)
				var y0: int = max(y - 1, 0)
				var y1: int = min(y + 1, size - 1)
				
				data[y * size + x] = (
					copy[y * size + x0] +
					copy[y * size + x1] +
					copy[y0 * size + x] +
					copy[y1 * size + x] +
					copy[y * size + x]
				) / 5
