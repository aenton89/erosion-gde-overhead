#include "PipeErosion.hpp"
#include <cmath>
#include <algorithm>

using namespace godot;



void PipeErosion::_bind_methods() {
    ClassDB::bind_method(D_METHOD("erode", "heightmap", "num_iterations", "config"), &PipeErosion::erode);
}



void PipeErosion::alloc(int size) {
    if (cached_size == size) 
        return;

    const int n = size * size;
    water.assign(n, 0.0f);
    flux_l.assign(n, 0.0f);
    flux_r.assign(n, 0.0f);
    flux_t.assign(n, 0.0f);
    flux_b.assign(n, 0.0f);

    sediment.assign(n, 0.0f);
    // pre-alokowany bufor transportu
    sediment_new.assign(n, 0.0f);
    terrain_prev.assign(n, 0.0f);

    vel_x.assign(n, 0.0f);
    vel_y.assign(n, 0.0f);
    cached_size = size;
}

// FAZA 1: flux
void PipeErosion::step_flux(const std::vector<float>& terrain, int size, const Ref<PipeErosionSettings>& cfg) {
    const float dt = cfg->get_dt();
    const float A = cfg->get_pipe_area();
    const float g = cfg->get_gravity();
    const float l = cfg->get_pipe_length();
    const float l2 = l * l;
    const float factor = dt * A * g / l;

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            const int i = idx(x, y, size);
            const float d = water[i];
            const float hc = terrain[i] + d;

            float fl = 0.0f, fr = 0.0f, ft = 0.0f, fb = 0.0f;

            if (x > 0) {
                int j = idx(x-1, y, size);
                fl = std::max(0.0f, flux_l[i] + factor * (hc - terrain[j] - water[j]));
            }
            if (x < size - 1) {
                int j = idx(x+1, y, size);
                fr = std::max(0.0f, flux_r[i] + factor * (hc - terrain[j] - water[j]));
            }
            if (y > 0) {
                int j = idx(x, y-1, size);
                ft = std::max(0.0f, flux_t[i] + factor * (hc - terrain[j] - water[j]));
            }
            if (y < size - 1) {
                int j = idx(x, y+1, size);
                fb = std::max(0.0f, flux_b[i] + factor * (hc - terrain[j] - water[j]));
            }

            float sum_out = fl + fr + ft + fb;
            if (sum_out > 1e-7f && d > 1e-7f) {
                // d*l² = objętość wody w komórce
                float scale = std::min(1.0f, (d * l2) / (sum_out * dt));
                fl *= scale;
                fr *= scale;
                ft *= scale;
                fb *= scale;
            } else {
                fl = fr = ft = fb = 0.0f;
            }

            flux_l[i] = fl;
            flux_r[i] = fr;
            flux_t[i] = ft;
            flux_b[i] = fb;
        }
    }
}

// FAZA 2: poziom wody i prędkość
void PipeErosion::step_water(int size, const Ref<PipeErosionSettings>& cfg) {
    const float dt = cfg->get_dt();
    const float l = cfg->get_pipe_length();
    const float l2 = l * l;
    const float max_vel = cfg->get_max_velocity();

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            const int i = idx(x, y, size);

            float fin_l = (x > 0) ? flux_r[idx(x - 1, y, size)] : 0.0f;
            float fin_r = (x < size - 1) ? flux_l[idx(x+1, y,   size)] : 0.0f;
            float fin_t = (y > 0) ? flux_b[idx(x, y - 1, size)] : 0.0f;
            float fin_b = (y < size - 1) ? flux_t[idx(x, y + 1, size)] : 0.0f;

            float sum_in = fin_l + fin_r + fin_t + fin_b;
            float sum_out = flux_l[i] + flux_r[i] + flux_t[i] + flux_b[i];

            float d_prev = water[i];
            float d_new = std::max(0.0f, d_prev + dt * (sum_in - sum_out) / l2);
            water[i] = d_new;

            float d_avg = (d_prev + d_new) * 0.5f;
            if (d_avg > 1e-4f) {
                float u = (fin_l - flux_l[i] + flux_r[i] - fin_r) / (2.0f * l * d_avg);
                float v = (fin_t - flux_t[i] + flux_b[i] - fin_b) / (2.0f * l * d_avg);
                // FIX: clamp prędkości - bez tego przy prawie-zerowej wodzie prędkość eksploduje i sedyment transport robi dziury
                vel_x[i] = std::max(-max_vel, std::min(max_vel, u));
                vel_y[i] = std::max(-max_vel, std::min(max_vel, v));
            } else {
                vel_x[i] = 0.0f;
                vel_y[i] = 0.0f;
            }
        }
    }
}

// FAZA 3: erozja i depozycja
void PipeErosion::step_erosion_deposition(std::vector<float>& terrain, int size, const Ref<PipeErosionSettings>& cfg) {
    const float Kc = cfg->get_Kc();
    const float Ks = cfg->get_Ks();
    const float Kd = cfg->get_Kd();
    const float min_tilt = cfg->get_min_tilt_sin();
    const float l = cfg->get_pipe_length();

    // pod wersję zoptymalizowaną: gradient ze snapshotu wejściowego, zapis w miejscu - zapewnia brak wyścigu
    terrain_prev.assign(terrain.begin(), terrain.end());

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            const int i = idx(x, y, size);

            // erozja zachodzi tylko gdy jest woda
            if (water[i] < 1e-4f) 
                continue;

            int xl = (x > 0) ? x-1 : 0;
            int xr = (x < size-1) ? x+1 : size-1;
            int yt = (y > 0) ? y - 1 : 0;
            int yb = (y < size-1) ? y+1 : size-1;

            float dbdx = (terrain_prev[idx(xr, y, size)] - terrain_prev[idx(xl, y, size)]) / (float(xr - xl) * l);
            float dbdy = (terrain_prev[idx(x, yb, size)] - terrain_prev[idx(x, yt, size)]) / (float(yb - yt) * l);

            float g2 = dbdx * dbdx + dbdy * dbdy;
            float sin_a = std::max(std::sqrt(g2) / std::sqrt(1.0f + g2), min_tilt);

            float speed = std::sqrt(vel_x[i] * vel_x[i] + vel_y[i] * vel_y[i]);
            float C = Kc * sin_a * speed;
            float s = sediment[i];

            if (C > s) {
                float delta = std::min(Ks * (C-s), terrain_prev[i]);
                terrain[i] = std::max(0.0f, terrain_prev[i] - delta);
                sediment[i] += delta;
            } else {
                float delta = std::min(Kd * (s-C), s);
                terrain[i] = std::max(0.0f, terrain_prev[i] + delta);
                sediment[i] -= delta;
            }

            terrain[i] = std::max(0.0f, terrain[i]);
        }
    }
}

// FAZA 4: transport sedymentu
void PipeErosion::step_sediment_transport(int size, const Ref<PipeErosionSettings>& cfg) {
    const float dt = cfg->get_dt();
    const float l = cfg->get_pipe_length();
    const float size_max = float(size-1) - 1e-4f;

    std::fill(sediment_new.begin(), sediment_new.end(), 0.0f);

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            const int i = idx(x, y, size);

            float px = std::max(0.0f, std::min(size_max, float(x) - vel_x[i] * dt / l));
            float py = std::max(0.0f, std::min(size_max, float(y) - vel_y[i] * dt / l));

            int x0 = int(px), y0 = int(py);
            int x1 = std::min(x0 + 1, size - 1), y1 = std::min(y0 + 1, size - 1);
            float fx = px - float(x0), fy = py - float(y0);

            sediment_new[i] = std::max(0.0f, 
                sediment[idx(x0, y0, size)] * (1 - fx) * (1 - fy)
                    + sediment[idx(x1, y0, size)] * fx * (1 - fy)
                    + sediment[idx(x0, y1, size)] * (1 - fx) * fy
                    + sediment[idx(x1, y1, size)] * fx * fy);
        }
    }

    std::swap(sediment, sediment_new);
}

// FAZA 5: parowanie
void PipeErosion::step_evaporation(std::vector<float>& terrain, int size, const Ref<PipeErosionSettings>& cfg) {
    const float factor = 1.0f - cfg->get_Ke() * cfg->get_dt();

    for (int i = 0; i < size * size; i++) {
        water[i] = std::max(0.0f, water[i] * factor);

        if (water[i] < 1e-5f && sediment[i] > 0.0f) {
            terrain[i] += sediment[i];
            sediment[i] = 0.0f;
        }
    }
}



// ERODE
void PipeErosion::erode(Ref<Heightmap> heightmap, int num_iterations, const Ref<PipeErosionSettings>& config) {
    if (heightmap.is_null() || heightmap->is_empty()) {
        ERR_PRINT("PipeErosion::erode - heightmap is null or empty");
        return;
    }

    const int size = heightmap->size;
    alloc(size);

    std::vector<float> terrain(heightmap->data.ptr(), heightmap->data.ptr() + size * size);

    const int rain_every = std::max(1, config->get_rain_iterations());
    const float rain = config->get_rain_rate();

    for (int iter = 0; iter < num_iterations; iter++) {
        if (iter % rain_every == 0) {
            for (int i = 0; i < size * size; i++){
                water[i] += rain * config->get_dt();
            }
        }

        step_flux(terrain, size, config);
        step_water(size, config);
        step_erosion_deposition(terrain, size, config);
        step_sediment_transport(size, config);
        step_evaporation(terrain, size, config);
    }

    {
        float* dst = heightmap->data.ptrw();
        for (int i = 0; i < size * size; i++){
            dst[i] = std::max(0.0f, terrain[i]);
        }
    }
}