#include "PerlinNoiseSettings.hpp"



void PerlinNoiseSettings::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_seed", "v"), &PerlinNoiseSettings::set_seed);
    ClassDB::bind_method(D_METHOD("get_seed"), &PerlinNoiseSettings::get_seed);
    ClassDB::bind_method(D_METHOD("set_octaves", "v"), &PerlinNoiseSettings::set_octaves);
    ClassDB::bind_method(D_METHOD("get_octaves"), &PerlinNoiseSettings::get_octaves);
    ClassDB::bind_method(D_METHOD("set_persistence", "v"), &PerlinNoiseSettings::set_persistence);
    ClassDB::bind_method(D_METHOD("get_persistence"), &PerlinNoiseSettings::get_persistence);
    ClassDB::bind_method(D_METHOD("set_lacunarity", "v"), &PerlinNoiseSettings::set_lacunarity);
    ClassDB::bind_method(D_METHOD("get_lacunarity"), &PerlinNoiseSettings::get_lacunarity);
    ClassDB::bind_method(D_METHOD("set_scale", "v"), &PerlinNoiseSettings::set_scale);
    ClassDB::bind_method(D_METHOD("get_scale"), &PerlinNoiseSettings::get_scale);

    ADD_GROUP("Perlin Noise Settings", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "seed"), "set_seed", "get_seed");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "octaves"), "set_octaves", "get_octaves");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "persistence"), "set_persistence", "get_persistence");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lacunarity"), "set_lacunarity", "get_lacunarity");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "scale"), "set_scale", "get_scale");
}



void PerlinNoiseSettings::set_seed(int v) { 
    seed = v; 
}

int PerlinNoiseSettings::get_seed() const { 
    return seed; 
}

void PerlinNoiseSettings::set_octaves(int v) { 
    octaves = v; 
}

int PerlinNoiseSettings::get_octaves() const { 
    return octaves; 
}

void PerlinNoiseSettings::set_persistence(float v) { 
    persistence = v; 
}

float PerlinNoiseSettings::get_persistence() const { 
    return persistence; 
}

void PerlinNoiseSettings::set_lacunarity(float v) { 
    lacunarity  = v; 
}

float PerlinNoiseSettings::get_lacunarity() const { 
    return lacunarity; 
}

void PerlinNoiseSettings::set_scale(float v) { 
    scale = v; 
}

float PerlinNoiseSettings::get_scale() const { 
    return scale; 
}