#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <vector>
#include "../../generation/Heightmap.hpp"
#include "../resources/PipeErosionSettings.hpp"

using namespace godot;



/*
- algorytm erozji przepływowej pipe-based, zoptymalizowana wersja z SIMD i wielowątkowością
- implementacja algorytmu Mei et al. "Fast Hydraulic Erosion Simulation and Visualization on GPU" (2007) (https://hal.inria.fr/inria-00402079/document)

- stan per-komórka:
    - wysokość terenu (z heightmapy)
    - głębokość wody d[i] (water)
    - przepływ przez rury (flux_l, flux_r, flux_t, flux_b)
    - osad w wodzie s[i] (sediment)
    - wektor prędkości, pochodna fluxu (vel_x, vel_y)
- 5 faz na każdą iterację, każda z nich to przejście po całej siatce
*/
class PipeErosionOptimized : public RefCounted {
    GDCLASS(PipeErosionOptimized, RefCounted);

protected:
    static void _bind_methods();

private:
    std::vector<float> water;
    std::vector<float> flux_l;
    std::vector<float> flux_r;
    std::vector<float> flux_t;
    std::vector<float> flux_b;
    std::vector<float> sediment;

    std::vector<float> sediment_new;
    std::vector<float> terrain_prev;

    std::vector<float> vel_x;
    std::vector<float> vel_y;

    int cached_size = -1;



    void alloc(int size);

    void step_flux(const std::vector<float>& terrain, int size, const Ref<PipeErosionSettings>& cfg);
    void step_water(int size, const Ref<PipeErosionSettings>& cfg);
    void step_erosion_deposition(std::vector<float>& terrain, int size, const Ref<PipeErosionSettings>& cfg);
    void step_sediment_transport(int size, const Ref<PipeErosionSettings>& cfg);
    void step_evaporation(std::vector<float>& terrain,int size, const Ref<PipeErosionSettings>& cfg);

    inline int idx(int x, int y, int size) const { 
        return y * size + x; 
    }

public:
    PipeErosionOptimized() = default;

    void erode(Ref<Heightmap> heightmap, int num_iterations, const Ref<PipeErosionSettings>& config);
};