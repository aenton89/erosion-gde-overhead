#pragma once

#include <godot_cpp/core/class_db.hpp>
#include <vector>
#include "GenerationSettings.hpp"

using namespace godot;



class PerlinNoiseSettings : public GenerationSettings {
    GDCLASS(PerlinNoiseSettings, GenerationSettings);

protected:
    static void _bind_methods();

public:
    int seed = 0;
    int octaves = 6;
    float persistence = 0.5f;
    float lacunarity = 2.0f;
    float scale = 0.005f;



    PerlinNoiseSettings() = default;

    void set_seed (int v);
    int get_seed() const;
    void set_octaves (int v);
    int get_octaves() const;
    void set_persistence (float v);
    float get_persistence() const;
    void set_lacunarity (float v);
    float get_lacunarity() const;
    void set_scale (float v);
    float get_scale() const;
};