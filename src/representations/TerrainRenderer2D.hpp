#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/tile_map_layer.hpp>
#include <godot_cpp/classes/tile_set.hpp>
#include <godot_cpp/classes/tile_set_atlas_source.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "../generation/Heightmap.hpp"

using namespace godot;



/*
 - generation of 2D representations from Heightmap - textures and tilemaps
 - takes Ref<Heightmap> from GDScript
*/
class TerrainRenderer2D : public RefCounted {
    GDCLASS(TerrainRenderer2D, RefCounted);

protected:
    static void _bind_methods();

public:
    Array colors;
    Array thresholds;
    int tile_size = 16;


    
    TerrainRenderer2D();

    // textures
    Ref<ImageTexture> generate_texture_gradient (Ref<Heightmap> heightmap, const Array& color_stops, const Array& height_thresholds);
    Ref<ImageTexture> generate_texture_colored (Ref<Heightmap> heightmap, const Array& colors, const Array& height_thresholds);
    Ref<ImageTexture> generate_texture_grayscale(Ref<Heightmap> heightmap);
    Ref<ImageTexture> generate_texture_from_config (Ref<Heightmap> heightmap, bool grayscale = false);

    // tilemap
    void generate_tilemap_layer (Ref<Heightmap> heightmap, TileMapLayer* layer, const Array& tile_ids, const Array& height_thresholds);
    void generate_colored_tilemap_layer (Ref<Heightmap> heightmap, TileMapLayer* layer, const Array& colors, const Array& height_thresholds, int tile_size = 16);
    void generate_grayscale_tilemap_layer(Ref<Heightmap> heightmap, TileMapLayer* layer, int levels = 8);
    void generate_tilemap_from_config (Ref<Heightmap> heightmap, TileMapLayer* layer, bool grayscale = false);
    Ref<TileSet> create_tileset(const Array& colors, const Array& height_thresholds, int tile_size = 16);

    // color helpers
    Color interpolate_color_gradient (int height, const Array& color_stops, const Array& height_thresholds);
    Color get_discrete_color (int height, const Array& colors, const Array& height_thresholds);
    int get_tile_id (int height, const Array& tile_ids, const Array& height_thresholds);

    void set_colors (const Array& v); 
    Array get_colors() const;
    void set_thresholds(const Array& v); 
    Array get_thresholds() const;
    void set_tile_size (int v);
    int get_tile_size() const;
};
