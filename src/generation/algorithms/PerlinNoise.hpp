#pragma once

#include <godot_cpp/core/class_db.hpp>
#include <vector>
#include "../resources/PerlinNoiseSettings.hpp"
#include "../Heightmap.hpp"

using namespace godot;



class PerlinNoise : public RefCounted {
    GDCLASS(PerlinNoise, RefCounted);

protected:
    static void _bind_methods();

private:
    std::vector<int> perm;


    
    void init_permutation(int seed);
    float fade(float t) const;
    float lerp(float a, float b, float t) const;
    float grad(int hash, float x, float y) const;
    float noise(float x, float y) const;

public:
    PerlinNoise() = default;

    Ref<Heightmap> generate(const Ref<PerlinNoiseSettings>& config);
};
