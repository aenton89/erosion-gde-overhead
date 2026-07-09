extends Node
class_name Helpers



static func save_terrain_binary(path: String, size: int, data: PackedFloat32Array) -> void:
	var file: = FileAccess.open(path, FileAccess.WRITE)
	if file == null:
		push_error("save_terrain_binary: cant open %s" % path)
		return
	
	file.store_32(size)
	file.store_buffer(data.to_byte_array())
	file.close()

static func load_terrain_size(path: String) -> int:
	var file: = FileAccess.open(path, FileAccess.READ)
	if file == null:
		push_error("load_terrain_size: cant open %s" % path)
		return 0
	
	var size: int = file.get_32()
	file.close()
	return size

static func load_terrain_data(path: String) -> PackedFloat32Array:
	var file: = FileAccess.open(path, FileAccess.READ)
	if file == null:
		push_error("load_terrain_data: cant open %s" % path)
		return PackedFloat32Array()
	
	var size: int = file.get_32()
	var bytes: PackedByteArray = file.get_buffer(size * size * 4)
	file.close()
	return bytes.to_float32_array()
