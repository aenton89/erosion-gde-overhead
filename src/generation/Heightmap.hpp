#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <vector>

using namespace godot;



/*
- keeps heightmap data and provides operations on it
- data stored as PackedFloat32Array (flat array, contiguous memory) instead of Array of Array (Variant boxing, non-contiguous)
- should be faster to access in generate_mesh, no per-row allocation
*/
class Heightmap : public RefCounted {
    GDCLASS(Heightmap, RefCounted);

protected:
    static void _bind_methods();

public:
    PackedFloat32Array data;
    int size = 0;


    
    Heightmap() = default;

    bool is_empty() const;
    void clear();

    // explicit access - one index instead of two Variant-indexing
    inline float get(int x, int y) const { return data[y * size + x]; }
    inline void set_value(int x, int y, float value) { data[y * size + x] = value; }

    // conversion from std::vector (used internally by generator)
    void from_vec(const std::vector<std::vector<float>>& vec);

    // operates on PackedFloat32 Array - without to_vec/from_vec
    void smooth(int iterations = 1);

    // getters for GDScript
    PackedFloat32Array get_data() const;
    int get_size() const;

    // setter for GDScript (unifing the base heightmap)
    void load_from_data(PackedFloat32Array new_data, int new_size);
};