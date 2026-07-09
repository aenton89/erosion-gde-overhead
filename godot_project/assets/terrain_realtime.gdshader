shader_type spatial;

// heightmap as texture
uniform sampler2D heightmap_tex : filter_linear, repeat_disable;
uniform float height_scale : hint_range(0.0, 200.0) = 20.0;
uniform float map_size : hint_range(1.0, 2048.0) = 512.0;

// based on: 0 = steepness, 1 = height
uniform bool use_height_color = false;

// steepness settings
uniform vec4 color_flat : source_color = vec4(0.25, 0.55, 0.15, 1.0);
uniform vec4 color_mid : source_color = vec4(0.45, 0.35, 0.22, 1.0);
uniform vec4 color_steep : source_color = vec4(0.32, 0.26, 0.20, 1.0);

uniform float flat_start : hint_range(0.0, 1.0) = 0.92;
uniform float flat_end : hint_range(0.0, 1.0) = 0.78;
uniform float rock_start : hint_range(0.0, 1.0) = 0.62;
uniform float rock_end : hint_range(0.0, 1.0) = 0.45;

// height settings
uniform vec4 h_color_0 : source_color = vec4(0.0, 0.0, 0.0, 1.0);
uniform vec4 h_color_1 : source_color = vec4(0.0, 0.0, 0.5, 1.0);
uniform vec4 h_color_2 : source_color = vec4(0.0, 0.0, 1.0, 1.0);
uniform vec4 h_color_3 : source_color = vec4(0.96, 0.64, 0.38, 1.0);
uniform vec4 h_color_4 : source_color = vec4(0.0, 1.0, 0.0, 1.0);
uniform vec4 h_color_5 : source_color = vec4(0.0, 0.5, 0.0, 1.0);
uniform vec4 h_color_6 : source_color = vec4(0.5, 0.5, 0.5, 1.0);
uniform vec4 h_color_7 : source_color = vec4(1.0, 1.0, 1.0, 1.0);

uniform float h_t0 : hint_range(0.0, 200.0) = 10.0;
uniform float h_t1 : hint_range(0.0, 200.0) = 20.0;
uniform float h_t2 : hint_range(0.0, 200.0) = 40.0;
uniform float h_t3 : hint_range(0.0, 200.0) = 60.0;
uniform float h_t4 : hint_range(0.0, 200.0) = 70.0;
uniform float h_t5 : hint_range(0.0, 200.0) = 80.0;
uniform float h_t6 : hint_range(0.0, 200.0) = 90.0;

// lighting
uniform float roughness : hint_range(0.0, 1.0) = 0.88;
uniform float metallic : hint_range(0.0, 1.0) = 0.0;


// passed from vertex, displacement
varying float v_height;
varying vec2 v_uv;



float sample_height(vec2 uv) {
	return texture(heightmap_tex, uv).r * height_scale;
}

vec3 calc_normal(vec2 uv) {
	// normal from central diff of neighbour texels
	float texel = 1.0 / map_size;
	float hL = sample_height(uv + vec2(-texel, 0.0));
	float hR = sample_height(uv + vec2( texel, 0.0));
	float hT = sample_height(uv + vec2(0.0, -texel));
	float hB = sample_height(uv + vec2(0.0, texel));
	return normalize(vec3(hL - hR, 2.0, hT - hB));
}

void vertex() {
	v_uv = UV;
	v_height = sample_height(UV);
	VERTEX.y = v_height;
	
	// normal calculated from texture, no need form generate_normals()
	NORMAL = calc_normal(UV);
}



vec3 height_color(float h) {
	if (h < h_t0) 
		return h_color_0.rgb;
	else if (h < h_t1) 
		return mix(h_color_0.rgb, h_color_1.rgb, smoothstep(h_t0, h_t1, h));
	else if (h < h_t2) 
		return mix(h_color_1.rgb, h_color_2.rgb, smoothstep(h_t1, h_t2, h));
	else if (h < h_t3) 
		return mix(h_color_2.rgb, h_color_3.rgb, smoothstep(h_t2, h_t3, h));
	else if (h < h_t4) 
		return mix(h_color_3.rgb, h_color_4.rgb, smoothstep(h_t3, h_t4, h));
	else if (h < h_t5) 
		return mix(h_color_4.rgb, h_color_5.rgb, smoothstep(h_t4, h_t5, h));
	else if (h < h_t6) 
		return mix(h_color_5.rgb, h_color_6.rgb, smoothstep(h_t5, h_t6, h));
	else
		return mix(h_color_6.rgb, h_color_7.rgb, smoothstep(h_t6, h_t6 + 10.0, h));
}

vec3 steepness_color(vec3 world_normal) {
	float slope = dot(world_normal, vec3(0.0, 1.0, 0.0));
	
	float t_mid = 1.0 - smoothstep(flat_end, flat_start, slope);
	float t_rock = 1.0 - smoothstep(rock_end, rock_start, slope);
	
	vec3 col = color_flat.rgb;
	col = mix(col, color_mid.rgb, t_mid);
	col = mix(col, color_steep.rgb, t_rock);
	return col;
}

void fragment() {
	vec3 world_normal = normalize((inverse(VIEW_MATRIX) * vec4(NORMAL, 0.0)).xyz);
	vec3 col = use_height_color ? height_color(v_height) : steepness_color(world_normal);
	
	ALBEDO = col;
	ROUGHNESS = roughness;
	METALLIC = metallic;
}
