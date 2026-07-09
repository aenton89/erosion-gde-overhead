#pragma once

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include "GenerationSettings.hpp"

using namespace godot;



class DiamondSquareSettings : public GenerationSettings {
    GDCLASS(DiamondSquareSettings, GenerationSettings);

protected:
    static void _bind_methods();

public:
    Vector2i random = Vector2i(-5, 5);
    int decrease_factor = 8;


    
    DiamondSquareSettings() = default;

    void set_random(Vector2i v);
    Vector2i get_random() const;
    void set_decrease_factor(int v); 
    int get_decrease_factor() const;
};