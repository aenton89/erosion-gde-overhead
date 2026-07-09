extends RefCounted
class_name TerrainMeshBuilderGDS



var height_scale: float = 1.0
var cell_size: float = 1.0



func generate_mesh(hmap: HeightmapGDS) -> ArrayMesh:
	var data: PackedFloat32Array = hmap.data
	var size: int = hmap.get_size()
	var st: SurfaceTool = SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	
	for y in range(size - 1):
		for x in range(size - 1):
			var h00: float = data[y * size + x] * height_scale
			var h10: float = data[y * size + (x + 1)] * height_scale
			var h01: float = data[(y + 1) * size + x] * height_scale
			var h11: float = data[(y + 1) * size + (x + 1)] * height_scale
			
			var v00: Vector3 = Vector3(x * cell_size, h00, y * cell_size)
			var v10: Vector3 = Vector3((x + 1) * cell_size, h10, y * cell_size)
			var v01: Vector3 = Vector3(x * cell_size, h01, (y + 1) * cell_size)
			var v11: Vector3 = Vector3((x + 1) * cell_size, h11, (y + 1) * cell_size)
			
			var uv00: Vector2 = Vector2(float(x) / (size - 1), float(y) / (size - 1))
			var uv10: Vector2 = Vector2(float(x + 1) / (size - 1), float(y) / (size - 1))
			var uv01: Vector2 = Vector2(float(x) / (size - 1), float(y + 1) / (size - 1))
			var uv11: Vector2 = Vector2(float(x + 1) / (size - 1), float(y + 1) / (size - 1))
			
			# trójkąt 1
			st.set_uv(uv00)
			st.add_vertex(v00)
			st.set_uv(uv10)
			st.add_vertex(v10)
			st.set_uv(uv11)
			st.add_vertex(v11)
			
			# trójkąt 2
			st.set_uv(uv00)
			st.add_vertex(v00)
			st.set_uv(uv11)
			st.add_vertex(v11)
			st.set_uv(uv01)
			st.add_vertex(v01)
	
	st.generate_normals()
	return st.commit()
