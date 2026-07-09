#pragma once

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <vector>

using namespace godot;



/*
- parametry dla erozji cząsteczkowej
- dla portu algorytmu Sebastiana Lague (https://github.com/SebLague/Hydraulic-Erosion)
*/
class HydraulicErosionSettings : public Resource {
    GDCLASS(HydraulicErosionSettings, Resource);

protected:
    static void _bind_methods();

public:
    int seed = 0;
    int erosion_radius = 3;
    float inertia = 0.05f;
    float sediment_capacity_factor = 4.0f;
    float min_sediment_capacity = 0.01f;
    float erode_speed = 0.3f;
    float deposit_speed = 0.3f;
    float evaporate_speed = 0.01f;
    float gravity = 4.0f;
    int max_droplet_lifetime = 30;
    float initial_water_volume = 1.0f;
    float initial_speed = 1.0f;


    
    HydraulicErosionSettings() = default;

    void set_seed(int v); 
    int get_seed() const;
    void set_erosion_radius(int v);
    int get_erosion_radius() const;
    void set_inertia(float v);
    float get_inertia() const;
    void set_sediment_capacity_factor(float v);
    float get_sediment_capacity_factor() const;
    void set_min_sediment_capacity(float v);
    float get_min_sediment_capacity() const;
    void set_erode_speed(float v);
    float get_erode_speed() const;
    void set_deposit_speed(float v);         
    float get_deposit_speed() const;
    void set_evaporate_speed(float v);
    float get_evaporate_speed() const;
    void set_gravity(float v);
    float get_gravity() const;
    void set_max_droplet_lifetime(int v);
    int get_max_droplet_lifetime() const;
    void set_initial_water_volume(float v);
    float get_initial_water_volume() const;
    void set_initial_speed(float v);
    float get_initial_speed() const;
};