extends RefCounted
class_name PerlinNoiseGeneratorGDS



func generate(config: PerlinNoiseSettings) -> HeightmapGDS:
	var size: int = config.map_size
	var noise: FastNoiseLite = FastNoiseLite.new()
	noise.seed = config.seed
	noise.noise_type = FastNoiseLite.TYPE_PERLIN
	noise.fractal_type = FastNoiseLite.FRACTAL_FBM
	noise.fractal_octaves = config.octaves
	noise.fractal_gain = config.persistence
	noise.fractal_lacunarity = config.lacunarity
	noise.frequency = config.scale
	
	# dwa przebiegi (jak w C++) - najpierw zbiera min/max, potem normalizacja
	var raw: PackedFloat32Array = PackedFloat32Array()
	raw.resize(size * size)
	var min_val: float = 1e9
	var max_val: float = -1e9
	
	for y in range(size):
		for x in range(size):
			var v: float = noise.get_noise_2d(float(x), float(y))
			raw[y * size + x] = v
			if v < min_val: 
				min_val = v
			if v > max_val: 
				max_val = v
	
	var hmap: HeightmapGDS = HeightmapGDS.new(size)
	var range_: float = max_val - min_val
	for y in range(size):
		for x in range(size):
			var t: float = (raw[y * size + x] - min_val) / range_
			var value: float = t * (config.height_range.y - config.height_range.x) + config.height_range.x
			#hmap.set_value(x, y, value)
			hmap.data[y * size + x] = value
	
	return hmap
