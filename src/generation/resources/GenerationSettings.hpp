#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;



// only config, without logic
class GenerationSettings : public Resource {
    GDCLASS(GenerationSettings, Resource);

protected:
    static void _bind_methods();

public:
    int map_size = 256;
    Vector2i height_range = Vector2i(0, 100);


    
    GenerationSettings() = default;

    void set_map_size(int v);
    int get_map_size() const;

    void set_height_range(Vector2i v);
    Vector2i get_height_range() const;
};