#include "HydraulicErosion.hpp"
#include <cmath>
#include <algorithm>

using namespace godot;



void HydraulicErosion::_bind_methods() {
    ClassDB::bind_method(D_METHOD("erode", "heightmap", "num_iterations", "config"), &HydraulicErosion::erode);
}



// precompute per-node erosion brush - done once and cached untill map_size and radius don't change
void HydraulicErosion::init_brush(int map_size, int radius) {
    if (cached_map_size == map_size && cached_erosion_radius == radius)
        return;

    brush_indices.assign(map_size * map_size, {});
    brush_weights.assign(map_size * map_size, {});

    const int n = map_size * map_size;

    for (int i = 0; i < n; i++) {
        int centre_x = i % map_size;
        int centre_y = i / map_size;

        float weight_sum = 0.0f;
        std::vector<int> tmp_idx;
        std::vector<float> tmp_w;

        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                float sqr_dst = float(dx * dx + dy * dy);
                if (sqr_dst >= float(radius*radius)) 
                    continue;

                int coord_x = centre_x+dx;
                int coord_y = centre_y+dy;
                if (coord_x < 0 || coord_x >= map_size || coord_y < 0 || coord_y >= map_size) 
                    continue;

                float w = 1.0f - std::sqrt(sqr_dst) / float(radius);
                weight_sum += w;
                tmp_idx.push_back(coord_y * map_size + coord_x);
                tmp_w.push_back(w);
            }
        }

        // normalizuj wagi
        if (weight_sum > 0.0f){
            for (auto& w : tmp_w) {
                w /= weight_sum;
            }
        }

        brush_indices[i] = std::move(tmp_idx);
        brush_weights[i] = std::move(tmp_w);
    }

    cached_map_size = map_size;
    cached_erosion_radius = radius;
}

// bilinear interpolation
HydraulicErosion::HeightAndGradient HydraulicErosion::calc_height_and_gradient(const std::vector<float>& map, int map_size, float pos_x, float pos_y) const {
    int coord_x = int(pos_x);
    int coord_y = int(pos_y);

    // offset wewnątrz komórki
    float x = pos_x - float(coord_x);
    float y = pos_y - float(coord_y);

    int idx_nw = coord_y * map_size + coord_x;
    float h_nw = map[idx_nw];
    float h_ne = map[idx_nw + 1];
    float h_sw = map[idx_nw + map_size];
    float h_se = map[idx_nw + map_size + 1];

    HeightAndGradient result;
    result.gradient_x = (h_ne - h_nw) * (1.0f - y) + (h_se - h_sw) * y;
    result.gradient_y = (h_sw - h_nw) * (1.0f - x) + (h_se - h_ne) * x;
    result.height = h_nw * (1.0f - x) * (1.0f - y) + h_ne * x * (1.0f - y) + h_sw * (1.0f - x) * y + h_se * x * y;
    return result;
}

void HydraulicErosion::erode(Ref<Heightmap> heightmap, int num_iterations, const Ref<HydraulicErosionSettings>& config) {
    if (heightmap.is_null() || heightmap->is_empty()) {
        ERR_PRINT("HydraulicErosion::erode - heightmap is null or empty");
        return;
    }

    const int map_size = heightmap->size;
    init_brush(map_size, config->get_erosion_radius());

    // Heightmap trzyma float - bezpośrednia kopia
    std::vector<float> map(heightmap->data.ptr(), heightmap->data.ptr() + map_size * map_size);

    if (current_seed != config->get_seed()) {
        rng.seed(config->get_seed());
        current_seed = config->get_seed();
    }
    // zawężony prawy koniec - interpolacja odczytuje sąsiada o indeksie o jeden większym + pierwsze wywołanie calc_height_and_gradient poprzedza sprawdzenie granic
    std::uniform_real_distribution<float> dist(0.0f, float(map_size - 1) - 1e-4f);

    for (int iter = 0; iter < num_iterations; iter++) {
        float pos_x = dist(rng);
        float pos_y = dist(rng);
        float dir_x = 0.0f, dir_y = 0.0f;
        float speed = config->get_initial_speed();
        float water = config->get_initial_water_volume();
        float sediment = 0.0f;

        for (int lifetime = 0; lifetime < config->get_max_droplet_lifetime(); lifetime++) {
            int node_x = int(pos_x);
            int node_y = int(pos_y);
            int droplet_idx = node_y * map_size + node_x;

            float cell_offset_x = pos_x - float(node_x);
            float cell_offset_y = pos_y - float(node_y);

            auto hg = calc_height_and_gradient(map, map_size, pos_x, pos_y);

            // aktualizuj kierunek i pozycję
            dir_x = dir_x * config->get_inertia() - hg.gradient_x * (1.0f - config->get_inertia());
            dir_y = dir_y * config->get_inertia() - hg.gradient_y * (1.0f - config->get_inertia());

            float len = std::sqrt(dir_x * dir_x + dir_y * dir_y);
            if (len != 0.0f) { 
                dir_x /= len; 
                dir_y /= len; 
            }

            pos_x += dir_x;
            pos_y += dir_y;

            // zatrzymaj kroplę jeśli nieruchoma lub poza mapą
            if ((dir_x == 0.0f && dir_y == 0.0f) || pos_x < 0.0f || pos_x >= float(map_size - 1) || pos_y < 0.0f || pos_y >= float(map_size - 1))
                break;

            float new_height = calc_height_and_gradient(map, map_size, pos_x, pos_y).height;
            float delta_height = new_height - hg.height;

            float sediment_capacity = std::max(-delta_height * speed * water * config->get_sediment_capacity_factor(), config->get_min_sediment_capacity());

            if (sediment > sediment_capacity || delta_height > 0.0f) {
                // depozycja
                float amount = (delta_height > 0.0f) ? std::min(delta_height, sediment) : (sediment - sediment_capacity) * config->get_deposit_speed();
                sediment -= amount;

                map[droplet_idx] += amount * (1.0f - cell_offset_x) * (1.0f - cell_offset_y);
                map[droplet_idx + 1] += amount * cell_offset_x * (1.0f - cell_offset_y);
                map[droplet_idx + map_size] += amount * (1.0f - cell_offset_x) * cell_offset_y;
                map[droplet_idx + map_size + 1] += amount * cell_offset_x * cell_offset_y;
            } else {
                // erozja
                float amount = std::min((sediment_capacity - sediment) * config->get_erode_speed(), -delta_height);

                const auto& b_idx = brush_indices[droplet_idx];
                const auto& b_w = brush_weights[droplet_idx];
                for (size_t bi = 0; bi < b_idx.size(); bi++) {
                    float weighted = amount * b_w[bi];
                    float delta = (map[b_idx[bi]] < weighted) ? map[b_idx[bi]] : weighted;
                    map[b_idx[bi]] -= delta;
                    sediment += delta;
                }
            }

            speed = std::sqrt(std::max(0.0f, speed * speed + delta_height * config->get_gravity()));
            water *= (1.0f - config->get_evaporate_speed());
        }
    }

    // zapis z powrotem do Heightmap
    {
        float* dst = heightmap->data.ptrw();
        for (int i = 0; i < map_size * map_size; i++){
            dst[i] = std::max(0.0f, map[i]);
        }
    }
}