#include "PipeErosionSettings.hpp"

using namespace godot;



void PipeErosionSettings::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_dt", "v"), &PipeErosionSettings::set_dt);
    ClassDB::bind_method(D_METHOD("get_dt"), &PipeErosionSettings::get_dt);
    ClassDB::bind_method(D_METHOD("set_pipe_length", "v"), &PipeErosionSettings::set_pipe_length);
    ClassDB::bind_method(D_METHOD("get_pipe_length"), &PipeErosionSettings::get_pipe_length);
    ClassDB::bind_method(D_METHOD("set_pipe_area", "v"), &PipeErosionSettings::set_pipe_area);
    ClassDB::bind_method(D_METHOD("get_pipe_area"), &PipeErosionSettings::get_pipe_area);
    ClassDB::bind_method(D_METHOD("set_gravity", "v"), &PipeErosionSettings::set_gravity);
    ClassDB::bind_method(D_METHOD("get_gravity"), &PipeErosionSettings::get_gravity);
    ClassDB::bind_method(D_METHOD("set_Kc", "v"), &PipeErosionSettings::set_Kc);
    ClassDB::bind_method(D_METHOD("get_Kc"), &PipeErosionSettings::get_Kc);
    ClassDB::bind_method(D_METHOD("set_Ks", "v"), &PipeErosionSettings::set_Ks);
    ClassDB::bind_method(D_METHOD("get_Ks"), &PipeErosionSettings::get_Ks);
    ClassDB::bind_method(D_METHOD("set_Kd", "v"), &PipeErosionSettings::set_Kd);
    ClassDB::bind_method(D_METHOD("get_Kd"), &PipeErosionSettings::get_Kd);
    ClassDB::bind_method(D_METHOD("set_Ke", "v"), &PipeErosionSettings::set_Ke);
    ClassDB::bind_method(D_METHOD("get_Ke"), &PipeErosionSettings::get_Ke);
    ClassDB::bind_method(D_METHOD("set_rain_rate", "v"), &PipeErosionSettings::set_rain_rate);
    ClassDB::bind_method(D_METHOD("get_rain_rate"), &PipeErosionSettings::get_rain_rate);
    ClassDB::bind_method(D_METHOD("set_rain_iterations", "v"), &PipeErosionSettings::set_rain_iterations);
    ClassDB::bind_method(D_METHOD("get_rain_iterations"), &PipeErosionSettings::get_rain_iterations);
    ClassDB::bind_method(D_METHOD("set_min_tilt_sin", "v"), &PipeErosionSettings::set_min_tilt_sin);
    ClassDB::bind_method(D_METHOD("get_min_tilt_sin"), &PipeErosionSettings::get_min_tilt_sin);
    ClassDB::bind_method(D_METHOD("set_max_velocity", "v"), &PipeErosionSettings::set_max_velocity);
    ClassDB::bind_method(D_METHOD("get_max_velocity"), &PipeErosionSettings::get_max_velocity);

    ADD_GROUP("Simulation", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dt"), "set_dt", "get_dt");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pipe_length"), "set_pipe_length", "get_pipe_length");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pipe_area"), "set_pipe_area", "get_pipe_area");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gravity"), "set_gravity", "get_gravity");
    ADD_GROUP("Sediment", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "Kc"), "set_Kc", "get_Kc");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "Ks"), "set_Ks", "get_Ks");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "Kd"), "set_Kd", "get_Kd");
    ADD_GROUP("Rain / Evaporation", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "Ke"), "set_Ke", "get_Ke");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "rain_rate"), "set_rain_rate", "get_rain_rate");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "rain_iterations"), "set_rain_iterations", "get_rain_iterations");
    ADD_GROUP("Stability", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_tilt_sin"), "set_min_tilt_sin", "get_min_tilt_sin");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_velocity"), "set_max_velocity", "get_max_velocity");
}

void PipeErosionSettings::set_dt(float v) {
    dt = v; 
}

float PipeErosionSettings::get_dt() const { 
    return dt; 
}

void PipeErosionSettings::set_pipe_length(float v) { 
    pipe_length = v; 
}

float PipeErosionSettings::get_pipe_length() const {
    return pipe_length; 
}

void PipeErosionSettings::set_pipe_area(float v) { 
    pipe_area = v; 
}

float PipeErosionSettings::get_pipe_area() const { 
    return pipe_area; 
}

void PipeErosionSettings::set_gravity(float v) { 
    gravity = v; 
}

float PipeErosionSettings::get_gravity() const { 
    return gravity; 
}

void PipeErosionSettings::set_Kc(float v) { 
    Kc = v; 
}

float PipeErosionSettings::get_Kc() const { 
    return Kc; 
}

void PipeErosionSettings::set_Ks(float v) { 
    Ks = v; 
}

float PipeErosionSettings::get_Ks() const { 
    return Ks; 
}

void PipeErosionSettings::set_Kd(float v) { 
    Kd = v; 
}

float PipeErosionSettings::get_Kd() const { 
    return Kd; 
}

void PipeErosionSettings::set_Ke(float v) { 
    Ke = v; 
}

float PipeErosionSettings::get_Ke() const { 
    return Ke; 
}

void PipeErosionSettings::set_rain_rate(float v) { 
    rain_rate = v; 
}

float PipeErosionSettings::get_rain_rate() const { 
    return rain_rate; 
}

void PipeErosionSettings::set_rain_iterations(int v) { 
    rain_iterations = v; 
}

int PipeErosionSettings::get_rain_iterations() const { 
    return rain_iterations; 
}

void PipeErosionSettings::set_min_tilt_sin(float v) { 
    min_tilt_sin = v; 
}

float PipeErosionSettings::get_min_tilt_sin() const { 
    return min_tilt_sin; 
}

void PipeErosionSettings::set_max_velocity(float v) { 
    max_velocity = v; 
}

float PipeErosionSettings::get_max_velocity() const {
    return max_velocity;
}