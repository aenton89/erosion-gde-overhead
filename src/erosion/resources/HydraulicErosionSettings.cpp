#include "HydraulicErosionSettings.hpp"



void HydraulicErosionSettings::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_seed", "v"), &HydraulicErosionSettings::set_seed);
    ClassDB::bind_method(D_METHOD("get_seed"), &HydraulicErosionSettings::get_seed);
    ClassDB::bind_method(D_METHOD("set_erosion_radius", "v"), &HydraulicErosionSettings::set_erosion_radius);
    ClassDB::bind_method(D_METHOD("get_erosion_radius"), &HydraulicErosionSettings::get_erosion_radius);
    ClassDB::bind_method(D_METHOD("set_inertia", "v"), &HydraulicErosionSettings::set_inertia);
    ClassDB::bind_method(D_METHOD("get_inertia"), &HydraulicErosionSettings::get_inertia);
    ClassDB::bind_method(D_METHOD("set_sediment_capacity_factor", "v"), &HydraulicErosionSettings::set_sediment_capacity_factor);
    ClassDB::bind_method(D_METHOD("get_sediment_capacity_factor"), &HydraulicErosionSettings::get_sediment_capacity_factor);
    ClassDB::bind_method(D_METHOD("set_min_sediment_capacity", "v"), &HydraulicErosionSettings::set_min_sediment_capacity);
    ClassDB::bind_method(D_METHOD("get_min_sediment_capacity"), &HydraulicErosionSettings::get_min_sediment_capacity);
    ClassDB::bind_method(D_METHOD("set_erode_speed", "v"), &HydraulicErosionSettings::set_erode_speed);
    ClassDB::bind_method(D_METHOD("get_erode_speed"), &HydraulicErosionSettings::get_erode_speed);
    ClassDB::bind_method(D_METHOD("set_deposit_speed", "v"), &HydraulicErosionSettings::set_deposit_speed);
    ClassDB::bind_method(D_METHOD("get_deposit_speed"), &HydraulicErosionSettings::get_deposit_speed);
    ClassDB::bind_method(D_METHOD("set_evaporate_speed", "v"), &HydraulicErosionSettings::set_evaporate_speed);
    ClassDB::bind_method(D_METHOD("get_evaporate_speed"), &HydraulicErosionSettings::get_evaporate_speed);
    ClassDB::bind_method(D_METHOD("set_gravity", "v"), &HydraulicErosionSettings::set_gravity);
    ClassDB::bind_method(D_METHOD("get_gravity"), &HydraulicErosionSettings::get_gravity);
    ClassDB::bind_method(D_METHOD("set_max_droplet_lifetime", "v"), &HydraulicErosionSettings::set_max_droplet_lifetime);
    ClassDB::bind_method(D_METHOD("get_max_droplet_lifetime"), &HydraulicErosionSettings::get_max_droplet_lifetime);
    ClassDB::bind_method(D_METHOD("set_initial_water_volume", "v"), &HydraulicErosionSettings::set_initial_water_volume);
    ClassDB::bind_method(D_METHOD("get_initial_water_volume"), &HydraulicErosionSettings::get_initial_water_volume);
    ClassDB::bind_method(D_METHOD("set_initial_speed", "v"), &HydraulicErosionSettings::set_initial_speed);
    ClassDB::bind_method(D_METHOD("get_initial_speed"), &HydraulicErosionSettings::get_initial_speed);

    ADD_GROUP("Erosion Settings", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "seed"), "set_seed", "get_seed");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "erosion_radius"), "set_erosion_radius", "get_erosion_radius");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "inertia"), "set_inertia", "get_inertia");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sediment_capacity_factor"), "set_sediment_capacity_factor", "get_sediment_capacity_factor");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_sediment_capacity"), "set_min_sediment_capacity", "get_min_sediment_capacity");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "erode_speed"), "set_erode_speed", "get_erode_speed");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "deposit_speed"), "set_deposit_speed", "get_deposit_speed");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "evaporate_speed"), "set_evaporate_speed", "get_evaporate_speed");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gravity"), "set_gravity", "get_gravity");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_droplet_lifetime"), "set_max_droplet_lifetime", "get_max_droplet_lifetime");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "initial_water_volume"), "set_initial_water_volume", "get_initial_water_volume");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "initial_speed"), "set_initial_speed", "get_initial_speed");
    
}



void HydraulicErosionSettings::set_seed(int v) { 
    seed = v; 
}

int HydraulicErosionSettings::get_seed() const { 
    return seed; 
}

void HydraulicErosionSettings::set_erosion_radius(int v) { 
    erosion_radius = v; 
}

int HydraulicErosionSettings::get_erosion_radius() const { 
    return erosion_radius; 
}

void HydraulicErosionSettings::set_inertia(float v) { 
    inertia = v; 
}

float HydraulicErosionSettings::get_inertia() const { 
    return inertia; 
}

void HydraulicErosionSettings::set_sediment_capacity_factor(float v) { 
    sediment_capacity_factor = v; 
}

float HydraulicErosionSettings::get_sediment_capacity_factor() const { 
    return sediment_capacity_factor; 
}

void HydraulicErosionSettings::set_min_sediment_capacity(float v) { 
    min_sediment_capacity = v; 
}

float HydraulicErosionSettings::get_min_sediment_capacity() const { 
    return min_sediment_capacity; 
}

void HydraulicErosionSettings::set_erode_speed(float v) { 
    erode_speed = v; 
}

float HydraulicErosionSettings::get_erode_speed() const { 
    return erode_speed; 
}

void HydraulicErosionSettings::set_deposit_speed(float v) { 
    deposit_speed = v; 
}

float HydraulicErosionSettings::get_deposit_speed() const { 
    return deposit_speed; 
}

void HydraulicErosionSettings::set_evaporate_speed(float v) { 
    evaporate_speed = v; 
}

float HydraulicErosionSettings::get_evaporate_speed() const { 
    return evaporate_speed; 
}

void HydraulicErosionSettings::set_gravity(float v) { 
    gravity = v; 
}

float HydraulicErosionSettings::get_gravity() const { 
    return gravity; 
}

void HydraulicErosionSettings::set_max_droplet_lifetime(int v) { 
    max_droplet_lifetime = v; 
}

int HydraulicErosionSettings::get_max_droplet_lifetime() const { 
    return max_droplet_lifetime; 
}

void HydraulicErosionSettings::set_initial_water_volume(float v) { 
    initial_water_volume = v; 
}

float HydraulicErosionSettings::get_initial_water_volume() const { 
    return initial_water_volume; 
}

void HydraulicErosionSettings::set_initial_speed(float v) { 
    initial_speed = v; 
}

float HydraulicErosionSettings::get_initial_speed() const { 
    return initial_speed; 
}