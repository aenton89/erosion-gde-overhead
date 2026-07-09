extends RefCounted
class_name TerrainRealtimeRenderer



var mesh_instance: MeshInstance3D
var shader_mat: ShaderMaterial
var image: Image
var texture: ImageTexture
var map_size: int
var height_scale: float



func setup_mesh_and_material(mesh_inst: MeshInstance3D, size: int, h_scale: float, shader_path: String = "res://assets/terrain_realtime.gdshader") -> void:
	mesh_instance = mesh_inst
	map_size = size
	height_scale = h_scale
	
	# płaska siatka generowana raz
	mesh_instance.mesh = create_flat_mesh(map_size)
	
	# tekstura R32F (jeden kanał float) - surowe wysokości, jednak bez normalizacji
	image = Image.create(map_size, map_size, false, Image.FORMAT_RF)
	texture = ImageTexture.create_from_image(image)
	
	# materiał
	shader_mat = ShaderMaterial.new()
	shader_mat.shader = load(shader_path)
	shader_mat.set_shader_parameter("heightmap_tex", texture)
	# to już tylko wizualny mnoznik, podawane sa surowe wartości
	shader_mat.set_shader_parameter("height_scale", height_scale)
	shader_mat.set_shader_parameter("map_size", float(map_size))
	mesh_instance.material_override = shader_mat

func set_use_height_color(value: bool) -> void:
	if shader_mat:
		shader_mat.set_shader_parameter("use_height_color", value)

# DEPRECATED: zbyt wolne, ale zostawie może dla ewentualnych porównań
# aktualizuje teksturę heightmapy (per klatka/N iteracji erozji)
#func update(heightmap: Heightmap) -> void:
	#if !texture or !image:
		#return
	#
	#var data: PackedFloat32Array = heightmap.get_data()
	#var n: int = map_size * map_size
	#var normalized: float
	#
	## normalizacja, żeby shader mógł pomnożyć przez height_scale*heights_max
	#for i in range(n):
		#normalized = data[i] / heights_max
		#image.set_pixel(i % map_size, i / map_size, Color(normalized, 0.0, 0.0, 1.0))
	#
	#texture.update(image)

# szybsza wersja - omija set_pixel i używa surowych bajtów, ale wymaga od get_data() normalizacji
func update_raw(data: PackedFloat32Array) -> void:
	if !texture:
		return
	# Image.FORMAT_RF trzyma surowe float32 - możemy przekonwertować bezpośrednio
	var byte_array: PackedByteArray = data.to_byte_array()
	image = Image.create_from_data(map_size, map_size, false, Image.FORMAT_RF, byte_array)
	texture.update(image)

# płaska siatka size×size quadsów z UV 0-1
func create_flat_mesh(size: int) -> ArrayMesh:
	var st: SurfaceTool = SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	
	var uv00: Vector2
	var uv10: Vector2
	var uv01: Vector2
	var uv11: Vector2
	
	var v00: Vector3
	var v10: Vector3
	var v01: Vector3
	var v11: Vector3
	
	for y in range(size - 1):
		for x in range(size - 1):
			uv00 = Vector2(float(x) / (size-1), float(y) / (size-1))
			uv10 = Vector2(float(x+1) / (size-1), float(y) / (size-1))
			uv01 = Vector2(float(x) / (size-1), float(y+1) / (size-1))
			uv11 = Vector2(float(x+1) / (size-1), float(y+1) / (size-1))
			
			v00 = Vector3(x, 0.0, y)
			v10 = Vector3(x+1, 0.0, y)
			v01 = Vector3(x, 0.0, y+1)
			v11 = Vector3(x+1, 0.0, y+1)
			
			st.set_uv(uv00); 
			st.add_vertex(v00)
			st.set_uv(uv10); 
			st.add_vertex(v10)
			st.set_uv(uv11); 
			st.add_vertex(v11)
			
			st.set_uv(uv00); 
			st.add_vertex(v00)
			st.set_uv(uv11); 
			st.add_vertex(v11)
			st.set_uv(uv01); 
			st.add_vertex(v01)
	
	# normalne liczy shader z tekstury
	return st.commit()
