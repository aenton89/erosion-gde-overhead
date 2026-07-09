#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <vector>
#include "../../generation/Heightmap.hpp"
#include "../resources/PipeErosionSettings.hpp"

using namespace godot;



/*
- algorytm erozji przepływowej pipe-based
- implementacja algorytmu Mei et al. "Fast Hydraulic Erosion Simulation and Visualization on GPU" (2007) (https://hal.inria.fr/inria-00402079/document)

- stan per-komórka:
    - wysokość terenu (z heightmapy)
    - głębokość wody d[i] (water)
    - przepływ przez rury (flux_l, flux_r, flux_t, flux_b)
    - osad w wodzie s[i] (sediment)
    - wektor prędkości, pochodna fluxu (vel_x, vel_y)
- 5 faz na każdą iterację, każda z nich to przejście po całej siatce
*/
class PipeErosion : public RefCounted {
    GDCLASS(PipeErosion, RefCounted);

protected:
    static void _bind_methods();

private:
    // d[i]
    std::vector<float> water;
    // f_L[i], f_R[i], f_T[i], f_B[i]
    std::vector<float> flux_l;
    std::vector<float> flux_r;
    std::vector<float> flux_t;
    std::vector<float> flux_b;
    // s[i]
    std::vector<float> sediment;

    // bufor transportu
    std::vector<float> sediment_new;
    // bufor poprzedniego terenu - potrzebny dla obliczeń bez konflitków zapisu
    std::vector<float> terrain_prev;
    
    // u[i], v[i]
    std::vector<float> vel_x;
    std::vector<float> vel_y;

    // pomocnicze 
    int cached_size = -1;



    void alloc(int size);

    // faza 1: aktualizacja fluxu między sąsiadami
    void step_flux(const std::vector<float>& terrain, int size, const Ref<PipeErosionSettings>& cfg);
    // faza 2: aktualizacja poziomu wody i wektora prędkości
    void step_water(int size, const Ref<PipeErosionSettings>& cfg);
    // faza 3: erozja i depozycja osadu
    void step_erosion_deposition(std::vector<float>& terrain, int size, const Ref<PipeErosionSettings>& cfg);
    // faza 4: transport osadu (semi-Lagrange)
    void step_sediment_transport(int size, const Ref<PipeErosionSettings>& cfg);
    // faza 5: parowanie
    void step_evaporation(std::vector<float>& terrain, int size, const Ref<PipeErosionSettings>& cfg);

    // inline helpers
    inline int idx(int x, int y, int size) const { 
        return y * size + x; 
    }

public:
    PipeErosion() = default;

    // główna metoda, modyfikuje heightmapę w miejscu
    void erode(Ref<Heightmap> heightmap, int num_iterations, const Ref<PipeErosionSettings>& config);
};