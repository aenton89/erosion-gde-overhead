#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "../generation/Heightmap.hpp"

using namespace godot;



/*
 - mesh generation for 3D terrain from Heightmap - with vertex colors
 - CHANGE: instead of SurfaceTool (per-vertex overhead, internal std::vector<Variant>)
 - direct filling of PackedArray and add_surface_from_arrays()
*/
class TerrainRenderer3D : public RefCounted {
    GDCLASS(TerrainRenderer3D, RefCounted);

protected:
    static void _bind_methods();

public:
    float height_scale = 1.0f;
    float cell_size = 1.0f;


    
    TerrainRenderer3D() = default;

    Ref<ArrayMesh> generate_mesh(Ref<Heightmap> heightmap);
    void set_mesh_on_node(Ref<Heightmap> heightmap, MeshInstance3D* mesh_instance);

    void set_height_scale(float v); 
    float get_height_scale() const;
    void set_cell_size (float v); 
    float get_cell_size() const;
};