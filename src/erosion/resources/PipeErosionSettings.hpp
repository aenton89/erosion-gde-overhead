#pragma once

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/resource.hpp>

using namespace godot;



/*
- parametry dla erozji pipe-based
- Mei et al. "Fast Hydraulic Erosion Simulation and Visualization on GPU" (2007) (https://hal.inria.fr/inria-00402079/document)
*/
class PipeErosionSettings : public Resource {
    GDCLASS(PipeErosionSettings, Resource);

protected:
    static void _bind_methods();

public:
    float dt = 0.02f;
    float pipe_length = 1.0f;
    float pipe_area = 0.1f;
    float gravity = 9.81f;

    // sediment capacity factor
    float Kc = 1.0f;
    // dissolving speed - pod erozje
    float Ks = 0.5f;
    // deposition speed - pod depozycja
    float Kd = 0.5f;

    // evaporation rate
    float Ke = 0.01f;
    float rain_rate = 0.01f;
    // co ile iteracji pada deszcz (1 = zawsze)
    int rain_iterations = 1;

    // minimalny sinus, zapobiega NaN na płaskim
    float min_tilt_sin = 0.05f;
    // clamp prędkości, zapobiega eksplozji przy d dążącym do 0
    float max_velocity = 10.0f;



    PipeErosionSettings() = default;

    void set_dt(float v);              
    float get_dt() const;
    void set_pipe_length(float v);
    float get_pipe_length() const;
    void set_pipe_area(float v);
    float get_pipe_area() const;
    void set_gravity(float v);
    float get_gravity() const;
    void set_Kc(float v);
    float get_Kc() const;
    void set_Ks(float v);
    float get_Ks() const;
    void set_Kd(float v);
    float get_Kd() const;
    void set_Ke(float v);
    float get_Ke() const;
    void set_rain_rate(float v);
    float get_rain_rate() const;
    void set_rain_iterations(int v);
    int get_rain_iterations() const;
    void set_min_tilt_sin(float v);
    float get_min_tilt_sin() const;
    void set_max_velocity(float v);
    float get_max_velocity() const;
};