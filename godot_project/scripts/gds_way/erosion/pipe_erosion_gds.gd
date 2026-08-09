extends RefCounted
class_name PipeErosionGDS



var _water: PackedFloat32Array
var _flux_l: PackedFloat32Array
var _flux_r: PackedFloat32Array
var _flux_t: PackedFloat32Array
var _flux_b: PackedFloat32Array
var _sediment: PackedFloat32Array
var _sediment_new: PackedFloat32Array
var _terrain_prev: PackedFloat32Array
var _vel_x: PackedFloat32Array
var _vel_y: PackedFloat32Array
var _cached_size: int = -1



func _alloc(size: int) -> void:
	if _cached_size == size:
		return
	
	var n: int = size * size
	
	_water = PackedFloat32Array()
	_water.resize(n)
	_flux_l = PackedFloat32Array()
	_flux_l.resize(n)
	_flux_r = PackedFloat32Array()
	_flux_r.resize(n)
	_flux_t = PackedFloat32Array()
	_flux_t.resize(n)
	_flux_b = PackedFloat32Array()
	_flux_b.resize(n)
	_sediment = PackedFloat32Array()
	_sediment.resize(n)
	_sediment_new = PackedFloat32Array()
	_sediment_new.resize(n)
	_terrain_prev = PackedFloat32Array()
	#_terrain_prev.resize(n)
	_vel_x = PackedFloat32Array()
	_vel_x.resize(n)
	_vel_y = PackedFloat32Array()
	_vel_y.resize(n)
	
	_cached_size = size

# 1st step
func _step_flux(terrain: PackedFloat32Array, size: int, cfg: PipeErosionSettingsGDS) -> void:
	var dt: float = cfg.dt
	var l: float = cfg.pipe_length
	var l2: float = l * l
	var factor: float = dt * cfg.pipe_area * cfg.gravity / l
	
	for y in range(size):
		for x in range(size):
			var i: int = y * size + x
			var d: float = _water[i]
			var hc: float = terrain[i] + d
			
			var fl: float = 0.0
			var fr: float = 0.0
			var ft: float = 0.0
			var fb: float = 0.0
			
			if x > 0:
				var j: int = y * size + (x-1)
				fl = maxf(0.0, _flux_l[i] + factor * (hc - terrain[j] - _water[j]))
			if x < size - 1:
				var j: int = y * size + (x+1)
				fr = maxf(0.0, _flux_r[i] + factor * (hc - terrain[j] - _water[j]))
			if y > 0:
				var j: int = (y-1) * size + x
				ft = maxf(0.0, _flux_t[i] + factor * (hc - terrain[j] - _water[j]))
			if y < size - 1:
				var j: int = (y+1) * size + x
				fb = maxf(0.0, _flux_b[i] + factor * (hc - terrain[j] - _water[j]))
			
			# FIX: skalowanie tylko gdy d > epsilon
			var sum_out: float = fl + fr + ft + fb
			if sum_out > 1e-7 and d > 1e-7:
				var scale: float = minf(1.0, (d * l2) / (sum_out * dt))
				fl *= scale
				fr *= scale
				ft *= scale
				fb *= scale
			else:
				fl = 0.0; fr = 0.0; ft = 0.0; fb = 0.0
			
			_flux_l[i] = fl; _flux_r[i] = fr
			_flux_t[i] = ft; _flux_b[i] = fb

# 2nd step
func _step_water(size: int, cfg: PipeErosionSettingsGDS) -> void:
	var dt: float = cfg.dt
	var l: float = cfg.pipe_length
	var l2: float = l * l
	var max_vel: float = cfg.max_velocity
	var inv_l2: float = dt / l2
	var inv_2l: float = 1.0 / (2.0 * l)
	
	for y in range(size):
		for x in range(size):
			var i: int = y * size + x
			
			var fin_l: float = _flux_r[y * size + (x-1)] if x > 0 else 0.0
			var fin_r: float = _flux_l[y * size + (x+1)] if x < size - 1 else 0.0
			var fin_t: float = _flux_b[(y-1) * size + x] if y > 0 else 0.0
			var fin_b: float = _flux_t[(y+1) * size + x] if y < size - 1 else 0.0
			
			var sum_in: float= fin_l + fin_r + fin_t + fin_b
			var sum_out: float = _flux_l[i] + _flux_r[i] + _flux_t[i] + _flux_b[i]
			
			var d_prev: float = _water[i]
			var d_new: float = maxf(0.0, d_prev + (sum_in - sum_out) * inv_l2)
			_water[i] = d_new
			
			var d_avg: float = (d_prev + d_new) * 0.5
			# FIX: próg 1e-4 zamiast 1e-6
			if d_avg > 1e-4:
				var u: float = (fin_l - _flux_l[i] + _flux_r[i] - fin_r) * inv_2l / d_avg
				var v: float = (fin_t - _flux_t[i] + _flux_b[i] - fin_b) * inv_2l / d_avg
				# FIX: clamp prędkości
				_vel_x[i] = clampf(u, -max_vel, max_vel)
				_vel_y[i] = clampf(v, -max_vel, max_vel)
			else:
				_vel_x[i] = 0.0
				_vel_y[i] = 0.0

# 3rd step
func _step_erosion_deposition(terrain: PackedFloat32Array, size: int, cfg: PipeErosionSettingsGDS) -> void:
	var Kc: float = cfg.Kc
	var Ks: float = cfg.Ks
	var Kd: float = cfg.Kd
	var min_tilt: float = cfg.min_tilt_sin
	var l: float = cfg.pipe_length
	
	# analogicznie do C++: gradient ze snapshotu, zapis w miejscu
	_terrain_prev = terrain.duplicate()
	
	for y in range(size):
		for x in range(size):
			var i: int = y * size + x
			
			# FIX: pominąć komórki bez wody
			if _water[i] < 1e-4:
				continue
			
			var xl: int = max(x-1, 0)
			var xr: int = min(x+1, size-1)
			var yt: int = max(y-1, 0)  
			var yb: int = min(y+1, size-1)
			
			var dbdx: float = (_terrain_prev[y*size+xr] - _terrain_prev[y*size+xl]) / (float(xr-xl) * l)
			var dbdy: float = (_terrain_prev[yb*size+x] - _terrain_prev[yt*size+x]) / (float(yb-yt) * l)
			
			var g2: float = dbdx*dbdx + dbdy*dbdy
			var sin_a: float = maxf(min_tilt, sqrt(g2) / sqrt(1.0 + g2))
			
			var speed: float = sqrt(_vel_x[i]*_vel_x[i] + _vel_y[i]*_vel_y[i])
			var C: float = Kc * sin_a * speed
			var s: float = _sediment[i]
			
			if C > s:
				var delta: float = minf(Ks * (C - s), _terrain_prev[i])
				terrain[i] = maxf(0.0, _terrain_prev[i] - delta)
				_sediment[i] += delta
			else:
				var delta: float = minf(Kd * (s - C), s)
				terrain[i] = maxf(0.0, _terrain_prev[i] + delta)
				_sediment[i] -= delta
			
			terrain[i] = maxf(0.0, terrain[i])

# 4th step
func _step_sediment_transport(size: int, cfg: PipeErosionSettingsGDS) -> void:
	var dt: float = cfg.dt
	var l: float = cfg.pipe_length
	
	#_sediment_new.resize(size * size)
	
	for y in range(size):
		for x in range(size):
			var i: int = y * size + x
			
			var px: float = float(x) - _vel_x[i] * dt / l
			var py: float = float(y) - _vel_y[i] * dt / l
			
			px = clampf(px, 0.0, float(size-1) - 1e-4)
			py = clampf(py, 0.0, float(size-1) - 1e-4)
			
			var x0: int = int(px)
			var y0: int = int(py)
			var x1: int = min(x0+1, size-1)
			var y1: int = min(y0+1, size-1)
			var fx: float = px - float(x0)
			var fy: float = py - float(y0)
			
			_sediment_new[i] = maxf(0.0,
				_sediment[y0*size+x0] * (1-fx) * (1-fy) +
				_sediment[y0*size+x1] * fx * (1-fy) +
				_sediment[y1*size+x0] * (1-fx) * fy +
				_sediment[y1*size+x1] * fx * fy)
	
	var tmp: PackedFloat32Array = _sediment
	_sediment = _sediment_new
	_sediment_new = tmp

# 5th step
func _step_evaporation(terrain: PackedFloat32Array, size: int, cfg: PipeErosionSettingsGDS) -> void:
	var factor: float = 1.0 - cfg.Ke * cfg.dt
	for i in range(size * size):
		_water[i] = maxf(0.0, _water[i] * factor)
		
		if _water[i] < 1e-5 and _sediment[i] > 0.0:
			terrain[i] += _sediment[i]
			_sediment[i] = 0.0

func erode(hmap: HeightmapGDS, num_iterations: int, config: PipeErosionSettingsGDS) -> void:
	var size: int = hmap.get_size()
	_alloc(size)
	
	var terrain: PackedFloat32Array = hmap.data.duplicate()
	#terrain.resize(size * size)
	
	#for y in range(size):
		#for x in range(size):
			##terrain[y * size + x] = hmap.get_value(x, y)
			#terrain[y * size + x] = hmap.data[y * size + x]
	
	var rain_every: int = max(1, config.rain_iterations)
	var rain_dt: float = config.rain_rate * config.dt
	
	for iter in range(num_iterations):
		if iter % rain_every == 0:
			for i in range(size * size):
				_water[i] += rain_dt
		
		_step_flux(terrain, size, config)
		_step_water(size, config)
		_step_erosion_deposition(terrain, size, config)
		_step_sediment_transport(size, config)
		_step_evaporation(terrain, size, config)
	
	for i in range(size * size):
		hmap.data[i] = maxf(0.0, terrain[i])
