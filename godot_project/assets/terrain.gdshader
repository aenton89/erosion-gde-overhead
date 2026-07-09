shader_type spatial;



// based on: 0 = steepness, 1 = height
uniform bool use_height_color = false;
uniform bool use_improved_steepness = false;



// steepness settings
uniform vec4 color_flat : source_color = vec4(0.25, 0.55, 0.15, 1.0);
uniform vec4 color_mid : source_color = vec4(0.45, 0.35, 0.22, 1.0);
uniform vec4 color_steep : source_color = vec4(0.32, 0.26, 0.20, 1.0);

uniform float flat_start : hint_range(0.0, 1.0) = 0.96;
uniform float flat_end : hint_range(0.0, 1.0) = 0.92;
uniform float rock_start : hint_range(0.0, 1.0) = 0.82;
uniform float rock_end : hint_range(0.0, 1.0) = 0.62;

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

// for improved steepness
uniform sampler2D noise_tex;
uniform float noise_strength : hint_range(0.0, 1.0) = 0.15;


// passed from vertex
varying float v_height;
varying vec3 v_world_pos;


void vertex() {
	v_height = VERTEX.y;
	
	v_world_pos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
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

vec3 improved_steepness_color(vec3 world_pos, vec3 world_normal, float height) {
	float slope = dot(world_normal, vec3(0.0, 1.0, 0.0));
	
	// noise; [-1, 1]
	float noise = texture(noise_tex, world_pos.xz * 0.02).r;
	noise = (noise - 0.5) * 2.0;
	
	// steepness - low terrain
	float t_mid_slope = 1.0 - smoothstep(flat_end, flat_start, slope);
	vec3 grass = color_flat.rgb;
	vec3 dirt  = color_mid.rgb;
	
	vec3 low_col = mix(grass, dirt, t_mid_slope);
	
	// rock transition
	float t1 = 50.0 + noise * 5.0;
	float mid_mask = smoothstep(t1 - 6.0, t1 + 6.0, height + 5.0);
	vec3 rock = vec3(0.38, 0.36, 0.36);
	
	vec3 mid_col = mix(dirt, rock, 0.65*(t_mid_slope + 0.8));
	
	vec3 col = mix(low_col, mid_col, mid_mask);
	
	// snow addition
	float t2 = 65.0 + noise * 8.0;
	float snow_mask = smoothstep(t2 - 8.0, t2 + 8.0, height);
	float slope_block = smoothstep(0.6, 0.9, slope);
	snow_mask *= (1.0 - slope_block);
	snow_mask = pow(snow_mask, 1.4);
	vec3 snow = vec3(0.92, 0.95, 1.0);
	
	col = mix(col, snow, snow_mask);
	
	return col;
}



void fragment() {
	// NORMAL jest w przestrzeni widoku - konwersja do przestrzeni świata
	vec3 world_normal = normalize((inverse(VIEW_MATRIX) * vec4(NORMAL, 0.0)).xyz);
	vec3 col = use_height_color ? height_color(v_height) : steepness_color(world_normal);
	if(use_improved_steepness){
		col = use_height_color ? height_color(v_height) : improved_steepness_color(v_world_pos, world_normal, v_height);;
	}
	
	ALBEDO = col;
	ROUGHNESS = roughness;
	METALLIC = metallic;
}