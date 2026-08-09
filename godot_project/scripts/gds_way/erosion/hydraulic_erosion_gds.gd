extends RefCounted
class_name HydraulicErosionGDS



# precomputed brush
var _brush_indices: Array[PackedInt32Array] = []
var _brush_weights: Array[PackedFloat32Array] = []
var _cached_map_size: int = -1
var _cached_erosion_radius: int = -1

# rng
var rng: RandomNumberGenerator = RandomNumberGenerator.new()
var _curr_seed: int = -1



func _init_brush(map_size: int, radius: int) -> void:
	if _cached_map_size == map_size and _cached_erosion_radius == radius:
		return
	
	var n: int = map_size * map_size
	_brush_indices.resize(n)
	_brush_weights.resize(n)
	
	for i in range(n):
		var centre_x: int = i % map_size
		var centre_y: int = i / map_size
		
		var tmp_idx: PackedInt32Array = PackedInt32Array()
		var tmp_w: PackedFloat32Array = PackedFloat32Array()
		var weight_sum: float = 0.0
		
		for dy in range(-radius, radius + 1):
			for dx in range(-radius, radius + 1):
				var sqr_dst: float = float(dx * dx + dy * dy)
				if sqr_dst >= float(radius * radius):
					continue
				
				var cx: int = centre_x + dx
				var cy: int = centre_y + dy
				if cx < 0 or cx >= map_size or cy < 0 or cy >= map_size:
					continue
				
				var w: float = 1.0 - sqrt(sqr_dst) / float(radius)
				weight_sum += w
				
				tmp_idx.append(cy * map_size + cx)
				tmp_w.append(w)
		
		if weight_sum > 0.0:
			for j in range(tmp_w.size()):
				tmp_w[j] /= weight_sum
		
		_brush_indices[i] = tmp_idx
		_brush_weights[i] = tmp_w
	
	_cached_map_size = map_size
	_cached_erosion_radius = radius

func _calc_hg(map: PackedFloat32Array, map_size: int, pos_x: float, pos_y: float) -> Vector3:
	var coord_x: int = int(pos_x)
	var coord_y: int = int(pos_y)
	var x: float = pos_x - float(coord_x)
	var y: float = pos_y - float(coord_y)
	
	var idx_nw: int = coord_y * map_size + coord_x
	var h_nw: float = map[idx_nw]
	var h_ne: float = map[idx_nw + 1]
	var h_sw: float = map[idx_nw + map_size]
	var h_se: float = map[idx_nw + map_size + 1]
	
	var grad_x: float = (h_ne - h_nw) * (1.0 - y) + (h_se - h_sw) * y
	var grad_y: float = (h_sw - h_nw) * (1.0 - x) + (h_se - h_ne) * x
	var height: float = h_nw * (1.0 - x) * (1.0 - y) + h_ne * x * (1.0 - y) + h_sw * (1.0 - x) * y + h_se * x * y
	
	return Vector3(height, grad_x, grad_y)

func erode(hmap: HeightmapGDS, num_iterations: int, config: HydraulicErosionSettingsGDS) -> void:
	var map_size: int = hmap.get_size()
	_init_brush(map_size, config.erosion_radius)
	
	# kopia danych jako PackedFloat32Array do szybkiej pracy
	var map: PackedFloat32Array = hmap.data.duplicate()
	
	if _curr_seed != config.seed:
		rng.seed = config.seed
		_curr_seed = config.seed
	
	for _iter in range(num_iterations):
		var pos_x: float = rng.randf_range(0.0, float(map_size - 1) - 0.0001)
		var pos_y: float = rng.randf_range(0.0, float(map_size - 1) - 0.0001)
		var dir_x: float = 0.0
		var dir_y: float = 0.0
		var speed: float = config.initial_speed
		var water: float = config.initial_water_volume
		var sediment: float = 0.0
		
		for _life in range(config.max_droplet_lifetime):
			var node_x: int = int(pos_x)
			var node_y: int = int(pos_y)
			var droplet_idx: int = node_y * map_size + node_x
			var cell_offset_x: float = pos_x - float(node_x)
			var cell_offset_y: float = pos_y - float(node_y)
			
			var hg: Vector3 = _calc_hg(map, map_size, pos_x, pos_y)
			var height: float = hg.x
			var grad_x: float = hg.y
			var grad_y: float = hg.z
			
			dir_x = dir_x * config.inertia - grad_x * (1.0 - config.inertia)
			dir_y = dir_y * config.inertia - grad_y * (1.0 - config.inertia)
			
			var length: float = sqrt(dir_x * dir_x + dir_y * dir_y)
			if length != 0.0:
				dir_x /= length
				dir_y /= length
			
			pos_x += dir_x
			pos_y += dir_y
			
			if (dir_x == 0.0 and dir_y == 0.0) or pos_x < 0.0 or pos_x >= float(map_size - 1) or pos_y < 0.0 or pos_y >= float(map_size - 1):
				break
			
			var new_height: float = _calc_hg(map, map_size, pos_x, pos_y).x
			var delta_height: float = new_height - height
			
			var sediment_capacity: float = maxf(-delta_height * speed * water * config.sediment_capacity_factor, config.min_sediment_capacity)
			
			if sediment > sediment_capacity or delta_height > 0.0:
				var amount: float = minf(delta_height, sediment) if delta_height > 0.0 else (sediment - sediment_capacity) * config.deposit_speed
				sediment -= amount
				
				map[droplet_idx] += amount * (1.0 - cell_offset_x) * (1.0 - cell_offset_y)
				map[droplet_idx + 1] += amount * cell_offset_x * (1.0 - cell_offset_y)
				map[droplet_idx + map_size] += amount * (1.0 - cell_offset_x) * cell_offset_y
				map[droplet_idx + map_size + 1] += amount * cell_offset_x * cell_offset_y
			else:
				var amount: float = minf((sediment_capacity - sediment) * config.erode_speed, -delta_height)
				
				var b_idx: PackedInt32Array = _brush_indices[droplet_idx]
				var b_w: PackedFloat32Array = _brush_weights[droplet_idx]
				for bi in range(b_idx.size()):
					var weighted: float = amount * b_w[bi]
					var delta: float = minf(map[b_idx[bi]], weighted)
					map[b_idx[bi]] -= delta
					sediment += delta
			
			speed = sqrt(maxf(0.0, speed * speed + delta_height * config.gravity))
			water *= (1.0 - config.evaporate_speed)
	
	# zapis z powrotem do heightmapy
	for i in range(map_size * map_size):
		hmap.data[i] = maxf(0.0, map[i])
	
	#for y in range(map_size):
		#for x in range(map_size):
			##hmap.set_value(x, y, maxf(0.0, map[y * map_size + x]))
			#hmap.data[y * map_size + x] = maxf(0.0, map[y * map_size + x])
