#include "PerlinNoise.hpp"
#include <algorithm>
#include <numeric>
#include <random>
#include <cmath>

using namespace godot;



void PerlinNoise::_bind_methods() {
    ClassDB::bind_method(D_METHOD("generate", "config"), &PerlinNoise::generate);
}



void PerlinNoise::init_permutation(int seed) {
    perm.resize(256);
    std::iota(perm.begin(), perm.end(), 0);
    std::mt19937 gen(seed);
    std::shuffle(perm.begin(), perm.end(), gen);
    // duplikacja - żeby uniknąć sprawdzania granic
    perm.insert(perm.end(), perm.begin(), perm.end());
}

float PerlinNoise::fade(float t) const {
    // smoothstep: 6t^5 - 15t^4 + 10t^3
    return t*t*t * (t * (t*6 - 15) + 10);
}

float PerlinNoise::lerp(float a, float b, float t) const {
    return a + t * (b - a);
}

float PerlinNoise::grad(int hash, float x, float y) const {
    switch (hash & 3) {
        case 0: 
            return x+y;
        case 1: 
            return y-x;
        case 2: 
            return x-y;
        case 3: 
            return -x-y;
        default: 
            return 0;
    }
}

float PerlinNoise::noise(float x, float y) const {
    int xi = int(std::floor(x)) & 255;
    int yi = int(std::floor(y)) & 255;

    float xf = x-std::floor(x);
    float yf = y-std::floor(y);

    float u = fade(xf);
    float v = fade(yf);

    int aa = perm[perm[xi] + yi];
    int ab = perm[perm[xi] + yi+1];
    int ba = perm[perm[xi+1] + yi];
    int bb = perm[perm[xi+1] + yi+1];

    return lerp(
        lerp(grad(aa, xf, yf), grad(ba, xf-1, yf), u),
        lerp(grad(ab, xf, yf-1), grad(bb, xf-1, yf-1), u),
        v
    );
}

Ref<Heightmap> PerlinNoise::generate(const Ref<PerlinNoiseSettings>& config) {
    init_permutation(config->get_seed());

    float min_val = 1e9f;
    float max_val = -1e9f;

    std::vector<std::vector<float>> raw(config->get_map_size(), std::vector<float>(config->get_map_size()));
    for (int y = 0; y < config->get_map_size(); y++) {
        for (int x = 0; x < config->get_map_size(); x++) {
            float amplitude = 1.0f;
            float frequency = 1.0f;
            float value = 0.0f;

            for (int o = 0; o < config->get_octaves(); o++) {
                float nx = x * config->get_scale() * frequency;
                float ny = y * config->get_scale() * frequency;
                value += noise(nx, ny) * amplitude;
                amplitude *= config->get_persistence();
                frequency *= config->get_lacunarity();
            }

            raw[y][x] = value;
            min_val = std::min(min_val, value);
            max_val = std::max(max_val, value);
        }
    }

    // normalizuj do zakresu heights
    float range = max_val - min_val;
    std::vector<std::vector<float>> vec(config->get_map_size(), std::vector<float>(config->get_map_size()));
    for (int y = 0; y < config->get_map_size(); y++){
        for (int x = 0; x < config->get_map_size(); x++){
            vec[y][x] = ((raw[y][x] - min_val) / range) * (config->get_height_range().y - config->get_height_range().x) + config->get_height_range().x;
        }
    }

    Ref<Heightmap> heightmap;
    heightmap.instantiate();
    heightmap->from_vec(vec);
    return heightmap;
}
