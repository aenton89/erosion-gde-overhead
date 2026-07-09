#include "TerrainRenderer2D.hpp"

using namespace godot;



TerrainRenderer2D::TerrainRenderer2D() {
    colors.append(Color(0, 0, 0));
    colors.append(Color(0, 0, 0.5));
    colors.append(Color(0, 0, 1));
    colors.append(Color(0.96, 0.64, 0.38));
    colors.append(Color(0, 1, 0));
    colors.append(Color(0, 0.5, 0));
    colors.append(Color(0.5, 0.5, 0.5));
    colors.append(Color(1, 1, 1));

    thresholds.append(10);
    thresholds.append(20);
    thresholds.append(40);
    thresholds.append(60);
    thresholds.append(70);
    thresholds.append(80);
    thresholds.append(90);
}

void TerrainRenderer2D::_bind_methods() {
    ClassDB::bind_method(D_METHOD("generate_texture_gradient", "heightmap", "color_stops", "height_thresholds"), &TerrainRenderer2D::generate_texture_gradient);
    ClassDB::bind_method(D_METHOD("generate_texture_colored", "heightmap", "colors", "height_thresholds"), &TerrainRenderer2D::generate_texture_colored);
    ClassDB::bind_method(D_METHOD("generate_texture_grayscale", "heightmap"), &TerrainRenderer2D::generate_texture_grayscale);
    ClassDB::bind_method(D_METHOD("generate_texture_from_config", "heightmap", "grayscale"), &TerrainRenderer2D::generate_texture_from_config);
    ClassDB::bind_method(D_METHOD("generate_tilemap_layer", "heightmap", "layer", "tile_ids", "height_thresholds"), &TerrainRenderer2D::generate_tilemap_layer);
    ClassDB::bind_method(D_METHOD("generate_colored_tilemap_layer", "heightmap", "layer", "colors",   "height_thresholds", "tile_size"), &TerrainRenderer2D::generate_colored_tilemap_layer);
    ClassDB::bind_method(D_METHOD("generate_grayscale_tilemap_layer", "heightmap", "layer", "levels"), &TerrainRenderer2D::generate_grayscale_tilemap_layer);
    ClassDB::bind_method(D_METHOD("generate_tilemap_from_config", "heightmap", "layer", "grayscale"), &TerrainRenderer2D::generate_tilemap_from_config);
    ClassDB::bind_method(D_METHOD("create_tileset", "colors", "height_thresholds", "tile_size"), &TerrainRenderer2D::create_tileset);

    ClassDB::bind_method(D_METHOD("set_colors", "colors"), &TerrainRenderer2D::set_colors);
    ClassDB::bind_method(D_METHOD("get_colors"), &TerrainRenderer2D::get_colors);
    ClassDB::bind_method(D_METHOD("set_thresholds", "thresholds"), &TerrainRenderer2D::set_thresholds);
    ClassDB::bind_method(D_METHOD("get_thresholds"), &TerrainRenderer2D::get_thresholds);
    ClassDB::bind_method(D_METHOD("set_tile_size", "size"), &TerrainRenderer2D::set_tile_size);
    ClassDB::bind_method(D_METHOD("get_tile_size"), &TerrainRenderer2D::get_tile_size);

    ADD_GROUP("Color Settings", "");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "colors", PROPERTY_HINT_ARRAY_TYPE, "Color"), "set_colors", "get_colors");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "thresholds", PROPERTY_HINT_ARRAY_TYPE, "int"), "set_thresholds", "get_thresholds");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "tile_size"), "set_tile_size", "get_tile_size");
}

Ref<ImageTexture> TerrainRenderer2D::generate_texture_gradient(Ref<Heightmap> heightmap, const Array& color_stops, const Array& height_thresholds) {
    if (heightmap.is_null() || heightmap->is_empty()) { 
        ERR_PRINT("Heightmap is empty."); 
        return {}; 
    }

    if (color_stops.size() != height_thresholds.size() || color_stops.size() < 2) {
        ERR_PRINT("color_stops and height_thresholds must have same size and at least 2 elements.");
        return {};
    }

    int size = heightmap->size;
    Ref<Image> image = Image::create(size, size, false, Image::FORMAT_RGB8);
    for (int y = 0; y < size; y++){
        for (int x = 0; x < size; x++){
            image->set_pixel(x, y, interpolate_color_gradient(heightmap->get(x, y), color_stops, height_thresholds));   
        }
    }
    Ref<ImageTexture> tex; 
    tex.instantiate(); 
    tex->set_image(image);

    return tex;
}

Ref<ImageTexture> TerrainRenderer2D::generate_texture_colored(Ref<Heightmap> heightmap, const Array& colors, const Array& height_thresholds) {
    if (heightmap.is_null() || heightmap->is_empty()) { 
        ERR_PRINT("Heightmap is empty."); 
        return {}; 
    }

    if (colors.size() != height_thresholds.size() + 1 || colors.size() < 2) {
        ERR_PRINT("colors must have one more element than height_thresholds and at least 2.");
        return {};
    }

    int size = heightmap->size;
    Ref<Image> image = Image::create(size, size, false, Image::FORMAT_RGB8);
    for (int y = 0; y < size; y++){
        for (int x = 0; x < size; x++){
            image->set_pixel(x, y, get_discrete_color(heightmap->get(x, y), colors, height_thresholds));
        }
    }
    Ref<ImageTexture> tex; 
    tex.instantiate(); 
    tex->set_image(image);
    
    return tex;
}

Ref<ImageTexture> TerrainRenderer2D::generate_texture_grayscale(Ref<Heightmap> heightmap) {
    if (heightmap.is_null() || heightmap->is_empty()) {
        ERR_PRINT("Heightmap is empty.");
        return {};
    }

    int size = heightmap->size;
    float min_h = 1e9f;
    float max_h = -1e9f;
    for (int y = 0; y < size; y++){
        for (int x = 0; x < size; x++) {
            float h = float(heightmap->get(x, y));
            if (h < min_h) 
                min_h = h;
            if (h > max_h) 
                max_h = h;
        }
    }
    float range = (max_h > min_h) ? (max_h - min_h) : 1.0f;

    Ref<Image> image = Image::create(size, size, false, Image::FORMAT_RGB8);
    for (int y = 0; y < size; y++){
        for (int x = 0; x < size; x++) {
            float t = (float(heightmap->get(x, y)) - min_h) / range;
            image->set_pixel(x, y, Color(t, t, t));
        }
    }

    Ref<ImageTexture> tex; 
    tex.instantiate(); 
    tex->set_image(image);
    
    return tex;
}

Ref<ImageTexture> TerrainRenderer2D::generate_texture_from_config(Ref<Heightmap> heightmap, bool grayscale) {
    if (grayscale)
        return generate_texture_grayscale(heightmap);
    else
        return generate_texture_colored(heightmap, colors, thresholds);
}

void TerrainRenderer2D::generate_tilemap_layer(Ref<Heightmap> heightmap, TileMapLayer* layer, const Array& tile_ids, const Array& height_thresholds) {
    if (heightmap.is_null() || heightmap->is_empty()) { 
        ERR_PRINT("Heightmap is empty."); 
        return; 
    }
    
    if (!layer) { 
        ERR_PRINT("TileMapLayer is null."); 
        return; 
    }

    if (tile_ids.size() != height_thresholds.size() + 1 || tile_ids.size() < 2) {
        ERR_PRINT("tile_ids must have one more element than height_thresholds.");
        return;
    }

    int size = heightmap->size;
    layer->clear();
    for (int y = 0; y < size; y++){
        for (int x = 0; x < size; x++){
            layer->set_cell(Vector2i(x, y), 0, Vector2i(get_tile_id(heightmap->get(x, y), tile_ids, height_thresholds), 0), 0);
        }
    }
}

void TerrainRenderer2D::generate_colored_tilemap_layer(Ref<Heightmap> heightmap, TileMapLayer* layer, const Array& colors, const Array& height_thresholds, int tile_size) {
    if (heightmap.is_null() || heightmap->is_empty()) { 
        ERR_PRINT("Heightmap is empty."); 
        return; 
    }
    
    if (!layer) { 
        ERR_PRINT("TileMapLayer is null."); 
        return; 
    }

    int size = heightmap->size;
    layer->clear();
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int h = heightmap->get(x, y);
            int tile_id = colors.size() - 1;
            for (int i = 0; i < height_thresholds.size(); i++){
                if (h <= int(height_thresholds[i])) { 
                    tile_id = i; 
                    break; 
                }
            }

            layer->set_cell(Vector2i(x, y), 0, Vector2i(tile_id, 0), 0);
        }
    }
}

void TerrainRenderer2D::generate_grayscale_tilemap_layer(Ref<Heightmap> heightmap, TileMapLayer* layer, int levels) {
    if (heightmap.is_null() || heightmap->is_empty()) {
        ERR_PRINT("Heightmap is empty.");
        return;
    }
    if (!layer) { 
        ERR_PRINT("TileMapLayer is null."); 
        return; 
    }
    levels = std::max(2, levels);

    int size = heightmap->size;
    int min_h = INT_MAX;
    int max_h = INT_MIN;
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++) {
            int h = heightmap->get(x, y);
            if (h < min_h) 
                min_h = h;
            if (h > max_h) 
                max_h = h;
        }

    Array gs_colors, gs_thresholds;
    float step = float(max_h - min_h) / float(levels);
    for (int i = 0; i < levels; i++) {
        float t = float(i) / float(levels - 1);
        gs_colors.append(Color(t, t, t));

        if (i < levels - 1)
            gs_thresholds.append(int(min_h + step * float(i + 1)));
    }

    Ref<TileSet> ts = create_tileset(gs_colors, gs_thresholds, tile_size);
    layer->set_tile_set(ts);
    generate_colored_tilemap_layer(heightmap, layer, gs_colors, gs_thresholds, tile_size);
}

Ref<TileSet> TerrainRenderer2D::create_tileset(const Array& colors, const Array& height_thresholds, int tile_size) {
    if (colors.size() != height_thresholds.size() + 1 || colors.size() < 2) {
        ERR_PRINT("colors must have one more element than height_thresholds.");
        return {};
    }

    Ref<TileSet> tileset;
    tileset.instantiate();
    tileset->set_tile_size(Vector2i(tile_size, tile_size));

    Ref<TileSetAtlasSource> atlas; 
    atlas.instantiate();
    Ref<Image> atlas_image = Image::create(colors.size() * tile_size, tile_size, false, Image::FORMAT_RGB8);

    for (int i = 0; i < colors.size(); i++) {
        Color c = colors[i];
        for (int py = 0; py < tile_size; py++){
            for (int px = 0; px < tile_size; px++){
                atlas_image->set_pixel(i * tile_size + px, py, c);
            }
        }
    }

    Ref<ImageTexture> atlas_tex; 
    atlas_tex.instantiate(); 
    atlas_tex->set_image(atlas_image);
    
    atlas->set_texture(atlas_tex);
    atlas->set_texture_region_size(Vector2i(tile_size, tile_size));
    atlas->set_margins(Vector2i(0, 0));
    atlas->set_separation(Vector2i(0, 0));

    for (int i = 0; i < colors.size(); i++){
        atlas->create_tile(Vector2i(i, 0));
    }

    tileset->add_source(atlas, 0);
    return tileset;
}

void TerrainRenderer2D::generate_tilemap_from_config(Ref<Heightmap> heightmap, TileMapLayer* layer, bool grayscale) {
    if (grayscale) 
        generate_grayscale_tilemap_layer(heightmap, layer, 8);
    else 
        generate_colored_tilemap_layer(heightmap, layer, colors, thresholds, tile_size);
}

Color TerrainRenderer2D::interpolate_color_gradient(int height, const Array& color_stops, const Array& height_thresholds) {
    for (int i = 0; i < height_thresholds.size() - 1; i++) {
        int lo = height_thresholds[i], hi = height_thresholds[i + 1];
        
        if (height >= lo && height <= hi) {
            float t = float(height - lo) / float(hi - lo);
            return Color(color_stops[i]).lerp(Color(color_stops[i + 1]), t);
        }
    }
    return height <= int(height_thresholds[0]) ? Color(color_stops[0]) : Color(color_stops[color_stops.size() - 1]);
}

Color TerrainRenderer2D::get_discrete_color(int height, const Array& colors, const Array& height_thresholds) {
    for (int i = 0; i < height_thresholds.size(); i++){
        if (height <= int(height_thresholds[i])) 
            return colors[i];
    }
    return colors[colors.size() - 1];
}

int TerrainRenderer2D::get_tile_id(int height, const Array& tile_ids, const Array& height_thresholds) {
    for (int i = 0; i < height_thresholds.size(); i++){
        if (height <= int(height_thresholds[i])) 
            return tile_ids[i];
    }
    return tile_ids[tile_ids.size() - 1];
}

void TerrainRenderer2D::set_colors(const Array& v) { 
    colors = v; 
}

Array TerrainRenderer2D::get_colors() const { 
    return colors; 
}

void TerrainRenderer2D::set_thresholds(const Array& v) { 
    thresholds = v; 
}

Array TerrainRenderer2D::get_thresholds() const { 
    return thresholds; 
}

void TerrainRenderer2D::set_tile_size(int v) { 
    tile_size = v; 
}

int TerrainRenderer2D::get_tile_size() const { 
    return tile_size; 
}
