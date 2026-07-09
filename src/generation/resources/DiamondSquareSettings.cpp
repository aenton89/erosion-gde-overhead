#include "DiamondSquareSettings.hpp"



void DiamondSquareSettings::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_random", "v"), &DiamondSquareSettings::set_random);
    ClassDB::bind_method(D_METHOD("get_random"), &DiamondSquareSettings::get_random);
    ClassDB::bind_method(D_METHOD("set_decrease_factor", "v"), &DiamondSquareSettings::set_decrease_factor);
    ClassDB::bind_method(D_METHOD("get_decrease_factor"), &DiamondSquareSettings::get_decrease_factor);

    ADD_GROUP("Diamond Square Settings", "");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "random"), "set_random", "get_random");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "decrease_factor"), "set_decrease_factor", "get_decrease_factor");
}



void DiamondSquareSettings::set_random(Vector2i v) { 
    random = v; 
}

Vector2i DiamondSquareSettings::get_random() const { 
    return random; 
}

void DiamondSquareSettings::set_decrease_factor(int v) { 
    decrease_factor = v; 
}

int DiamondSquareSettings::get_decrease_factor() const { 
    return decrease_factor; 
}