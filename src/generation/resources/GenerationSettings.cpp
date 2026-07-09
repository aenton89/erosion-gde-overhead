#include "GenerationSettings.hpp"



void GenerationSettings::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_map_size", "v"), &GenerationSettings::set_map_size);
    ClassDB::bind_method(D_METHOD("get_map_size"), &GenerationSettings::get_map_size);

    ClassDB::bind_method(D_METHOD("set_height_range", "v"), &GenerationSettings::set_height_range);
    ClassDB::bind_method(D_METHOD("get_height_range"), &GenerationSettings::get_height_range);

    ADD_GROUP("Generation Settings", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "map_size"), "set_map_size", "get_map_size");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "height_range"), "set_height_range", "get_height_range");
}



void GenerationSettings::set_map_size(int v) { 
    map_size = v; 
}

int GenerationSettings::get_map_size() const { 
    return map_size; 
}

void GenerationSettings::set_height_range(Vector2i v) { 
    height_range = v; 
}

Vector2i GenerationSettings::get_height_range() const { 
    return height_range; 
}