#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <vector>
#include <random>
#include "../../generation/Heightmap.hpp"
#include "../resources/HydraulicErosionSettings.hpp"

using namespace godot;



/*
- algorytm erozji cząsteczkowej
- port implementacji Sebastiana Lague (https://github.com/SebLague/Hydraulic-Erosion)
*/
class HydraulicErosion : public RefCounted {
    GDCLASS(HydraulicErosion, RefCounted);

protected:
    static void _bind_methods();

private:
    // precomputed brush per node
    std::vector<std::vector<int>> brush_indices;
    std::vector<std::vector<float>> brush_weights;

    int cached_map_size = -1;
    int cached_erosion_radius = -1;

    std::mt19937 rng;
    int current_seed = -1;


    
    void init_brush(int map_size, int radius);

    struct HeightAndGradient {
        float height = 0;
        float gradient_x = 0;
        float gradient_y = 0;
    };

    HeightAndGradient calc_height_and_gradient(const std::vector<float>& map, int map_size, float pos_x, float pos_y) const;

public:
    HydraulicErosion() = default;

    // główna metoda, modyfikuje heightmapę w miejscu
    void erode(Ref<Heightmap> heightmap, int num_iterations, const Ref<HydraulicErosionSettings>& config);
};
