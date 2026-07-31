#include "PipeErosionOptimized.hpp"
#include <cmath>
#include <algorithm>
#include <omp.h>
#include <hwy/highway.h>

namespace hn = hwy::HWY_NAMESPACE;

using namespace godot;



void PipeErosionOptimized::_bind_methods() {
    ClassDB::bind_method(D_METHOD("erode", "heightmap", "num_iterations", "config"), &PipeErosionOptimized::erode);
}



void PipeErosionOptimized::alloc(int size) {
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
void PipeErosionOptimized::step_flux(const std::vector<float>& terrain, int size, const Ref<PipeErosionSettings>& cfg) {
    const float dt = cfg->get_dt();
    const float A = cfg->get_pipe_area();
    const float g = cfg->get_gravity();
    const float l = cfg->get_pipe_length();
    const float l2 = l * l;
    const float factor = dt * A * g / l;

    // __restrict informuje kompilator że pointery nie aliasują (lepszy codegen)
    const float* __restrict ter = terrain.data();
    const float* __restrict wat = water.data();

    const hn::ScalableTag<float> d_tag;
    const int W = static_cast<int>(hn::Lanes(d_tag));

    const auto v_factor = hn::Set(d_tag, factor);
    const auto v_zero = hn::Zero(d_tag);
    const auto v_one = hn::Set(d_tag, 1.0f);
    const auto v_l2 = hn::Set(d_tag, l2);
    const auto v_dt = hn::Set(d_tag, dt);
    const auto v_eps = hn::Set(d_tag, 1e-7f);

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < size; y++) {

        auto scalar = [&](int x) {
            const int i = y * size + x;
            const float d = wat[i];
            const float hc = ter[i] + d;
            float fl = 0.0f, fr = 0.0f, ft = 0.0f, fb = 0.0f;
            
            if (x > 0) { 
                int j = i - 1;
                fl = std::max(0.0f, flux_l[i] + factor * (hc - ter[j] - wat[j])); 
            }
            if (x < size-1) { 
                int j = i + 1;
                fr = std::max(0.0f, flux_r[i] + factor * (hc - ter[j] - wat[j])); 
            }
            if (y > 0) { 
                int j = i - size; 
                ft = std::max(0.0f, flux_t[i] + factor * (hc - ter[j] - wat[j])); 
            }
            if (y < size-1) { 
                int j = i + size; 
                fb = std::max(0.0f, flux_b[i] + factor * (hc - ter[j] - wat[j])); 
            }

            float sum_out = fl + fr + ft + fb;
            if (sum_out > 1e-7f && d > 1e-7f) {
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
        };

        scalar(0);

        if (y > 0 && y < size - 1) {
            int x = 1;
            for (; x + W <= size - 1; x += W) {
                const int i = y * size + x;

                auto d_v = hn::LoadU(d_tag, wat + i);
                auto hc_v = hn::Add(hn::LoadU(d_tag, ter + i), d_v);

                auto hL = hn::Add(hn::LoadU(d_tag, ter + i - 1), hn::LoadU(d_tag, wat + i - 1));
                auto hR = hn::Add(hn::LoadU(d_tag, ter + i + 1), hn::LoadU(d_tag, wat + i + 1));
                auto hT = hn::Add(hn::LoadU(d_tag, ter + i - size), hn::LoadU(d_tag, wat + i - size));
                auto hB = hn::Add(hn::LoadU(d_tag, ter + i + size), hn::LoadU(d_tag, wat + i + size));

                auto fl_p = hn::LoadU(d_tag, flux_l.data() + i);
                auto fr_p = hn::LoadU(d_tag, flux_r.data() + i);
                auto ft_p = hn::LoadU(d_tag, flux_t.data() + i);
                auto fb_p = hn::LoadU(d_tag, flux_b.data() + i);

                auto fl = hn::Max(v_zero, hn::Add(fl_p, hn::Mul(v_factor, hn::Sub(hc_v, hL))));
                auto fr = hn::Max(v_zero, hn::Add(fr_p, hn::Mul(v_factor, hn::Sub(hc_v, hR))));
                auto ft = hn::Max(v_zero, hn::Add(ft_p, hn::Mul(v_factor, hn::Sub(hc_v, hT))));
                auto fb = hn::Max(v_zero, hn::Add(fb_p, hn::Mul(v_factor, hn::Sub(hc_v, hB))));

                auto sum_out = hn::Add(hn::Add(fl, fr), hn::Add(ft, fb));
                auto mask = hn::And(hn::Gt(sum_out, v_eps), hn::Gt(d_v, v_eps));
                auto scale = hn::IfThenElseZero(mask, hn::Min(v_one, hn::Div(hn::Mul(d_v, v_l2), hn::Mul(sum_out, v_dt))));

                hn::StoreU(hn::Mul(fl, scale), d_tag, flux_l.data() + i);
                hn::StoreU(hn::Mul(fr, scale), d_tag, flux_r.data() + i);
                hn::StoreU(hn::Mul(ft, scale), d_tag, flux_t.data() + i);
                hn::StoreU(hn::Mul(fb, scale), d_tag, flux_b.data() + i);
            }
    
            for (; x < size-1; x++) {
                scalar(x);
            }
    
        } else {
            for (int x = 1; x < size-1; x++) {
                scalar(x);
            }
        }

        scalar(size-1);
    }
}


// FAZA 2: poziom wody i prędkość
// gather z sąsiednich wierszy; SIMD nie pomaga, tylko OpenMP
void PipeErosionOptimized::step_water(int size, const Ref<PipeErosionSettings>& cfg) {
    const float dt = cfg->get_dt();
    const float l = cfg->get_pipe_length();
    const float l2 = l * l;
    const float max_vel = cfg->get_max_velocity();
    const float inv_l2 = dt / l2;
    const float inv_2l = 1.0f / (2.0f * l);

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            const int i = y * size + x;

            float fin_l = (x > 0) ? flux_r[i - 1] : 0.0f;
            float fin_r = (x < size-1) ? flux_l[i + 1] : 0.0f;
            float fin_t = (y > 0) ? flux_b[i - size] : 0.0f;
            float fin_b = (y < size-1) ? flux_t[i + size] : 0.0f;

            float sum_in  = fin_l + fin_r + fin_t + fin_b;
            float sum_out = flux_l[i] + flux_r[i] + flux_t[i] + flux_b[i];

            float d_prev = water[i];
            float d_new = std::max(0.0f, d_prev + (sum_in - sum_out) * inv_l2);
            water[i] = d_new;

            float d_avg  = (d_prev + d_new) * 0.5f;
            if (d_avg > 1e-4f) {
                float u = (fin_l - flux_l[i] + flux_r[i] - fin_r) * inv_2l / d_avg;
                float v = (fin_t - flux_t[i] + flux_b[i] - fin_b) * inv_2l / d_avg;
                vel_x[i] = std::max(-max_vel, std::min(max_vel, u));
                vel_y[i] = std::max(-max_vel, std::min(max_vel, v));
            } else {
                vel_x[i] = vel_y[i] = 0.0f;
            }
        }
    }
}

// FAZA 3: erozja i depozycja
// dynamic schedule bo suche wiersze z AllFalse są szybsze - lepszy load balancing
void PipeErosionOptimized::step_erosion_deposition(std::vector<float>& terrain, int size, const Ref<PipeErosionSettings>& cfg) {
    const float Kc = cfg->get_Kc();
    const float Ks = cfg->get_Ks();
    const float Kd = cfg->get_Kd();
    const float min_tilt = cfg->get_min_tilt_sin();
    const float l = cfg->get_pipe_length();
    const float inv_2l = 0.5f / l;

    terrain_prev.assign(terrain.begin(), terrain.end());
    const float* __restrict ter_in = terrain_prev.data();
    float* __restrict ter = terrain.data();

    const hn::ScalableTag<float> d_tag;
    const int W = static_cast<int>(hn::Lanes(d_tag));

    const auto v_Kc = hn::Set(d_tag, Kc);
    const auto v_Ks = hn::Set(d_tag, Ks);
    const auto v_Kd = hn::Set(d_tag, Kd);
    const auto v_mt = hn::Set(d_tag, min_tilt);
    const auto v_i2l = hn::Set(d_tag, inv_2l);
    const auto v_zero = hn::Zero(d_tag);
    const auto v_one = hn::Set(d_tag, 1.0f);
    const auto v_eps = hn::Set(d_tag, 1e-4f);

    auto scalar_border = [&](int x, int y) {
        const int i = y * size + x;
        if (water[i] < 1e-4f)
            return;

        int xl = (x > 0) ? x - 1 : 0;
        int xr = (x < size - 1) ? x + 1 : size - 1;
        int yt = (y > 0) ? y - 1 : 0;
        int yb = (y < size - 1) ? y + 1 : size - 1;

        float dbdx = (ter_in[y * size + xr] - ter_in[y * size + xl]) / (float(xr - xl) * l);
        float dbdy = (ter_in[yb * size + x] - ter_in[yt * size + x]) / (float(yb - yt) * l);

        float g2 = dbdx * dbdx + dbdy * dbdy;
        float sin_a = std::max(min_tilt, std::sqrt(g2) / std::sqrt(1.0f + g2));

        float speed = std::sqrt(vel_x[i] * vel_x[i] + vel_y[i] * vel_y[i]);
        float C = Kc * sin_a * speed;
        float s = sediment[i];

        if (C > s) {
            float delta = std::min(Ks * (C-s), ter_in[i]);
            ter[i] = std::max(0.0f, ter_in[i] - delta);
            sediment[i] += delta;
        } else {
            float delta = std::min(Kd * (s-C), s);
            ter[i] = std::max(0.0f, ter_in[i] + delta);
            sediment[i] -= delta;
        }
    };

    #pragma omp parallel
    {
        // górny i dolny brzeg mapy, obsłużony osobno
        #pragma omp for schedule(static) nowait
        for (int x = 0; x < size; x++) {
            scalar_border(x, 0);
            scalar_border(x, size - 1);
        }

        // właśnie tu dynamic (wiersze suche skończą szybciej, wątki dostaną więcej pracy)
        #pragma omp for schedule(dynamic, 4)
        for (int y = 1; y < size - 1; y++) {
            auto scalar = [&](int x) {
                const int i = y * size + x;
                if (water[i] < 1e-4f) 
                    return;

                float dbdx = (ter_in[i + 1] - ter_in[i - 1]) * inv_2l;
                float dbdy = (ter_in[i + size] - ter_in[i - size]) * inv_2l;

                float g2 = dbdx * dbdx + dbdy * dbdy;
                float sin_a = std::max(min_tilt, std::sqrt(g2) / std::sqrt(1.0f + g2));

                float speed = std::sqrt(vel_x[i] * vel_x[i] + vel_y[i] * vel_y[i]);
                float C = Kc * sin_a * speed;
                float s = sediment[i];

                if (C > s) {
                    float delta = std::min(Ks * (C-s), ter_in[i]);
                    ter[i] = std::max(0.0f, ter_in[i] - delta); 
                    sediment[i] += delta;
                } else {
                    float delta = std::min(Kd * (s-C), s);
                    ter[i] = std::max(0.0f, ter_in[i] + delta); 
                    sediment[i] -= delta;
                }
            };

            // lewa kolumna
            scalar_border(0, y);
            
            scalar(1);

            int x = 2;
            for (; x + W <= size - 1; x += W) {
                const int i = y * size + x;

                auto w = hn::LoadU(d_tag, water.data() + i);
                auto wet = hn::Gt(w, v_eps);
                if (hn::AllFalse(d_tag, wet)) 
                    continue;

                auto tL = hn::LoadU(d_tag, ter_in + i - 1);
                auto tR = hn::LoadU(d_tag, ter_in + i + 1);
                auto tT = hn::LoadU(d_tag, ter_in + i - size);
                auto tB = hn::LoadU(d_tag, ter_in + i + size);
                auto ti = hn::LoadU(d_tag, ter_in + i);
                auto s = hn::LoadU(d_tag, sediment.data() + i);
                auto vx = hn::LoadU(d_tag, vel_x.data() + i);
                auto vy = hn::LoadU(d_tag, vel_y.data() + i);

                auto dbdx = hn::Mul(hn::Sub(tR, tL), v_i2l);
                auto dbdy = hn::Mul(hn::Sub(tB, tT), v_i2l);
                auto g2 = hn::Add(hn::Mul(dbdx, dbdx), hn::Mul(dbdy, dbdy));
                auto grad = hn::Sqrt(g2);
                auto sin_a = hn::Max(v_mt, hn::Div(grad, hn::Sqrt(hn::Add(v_one, g2))));
                auto speed = hn::Sqrt(hn::Add(hn::Mul(vx, vx), hn::Mul(vy, vy)));
                auto C = hn::Mul(v_Kc, hn::Mul(sin_a, speed));
                auto erode = hn::Gt(C, s);

                auto d_e = hn::Min(hn::Mul(v_Ks, hn::Sub(C, s)), ti);
                auto d_d = hn::Min(hn::Mul(v_Kd, hn::Sub(s, C)), s);
                auto ti_new = hn::Max(v_zero, hn::IfThenElse(erode, hn::Sub(ti, d_e), hn::Add(ti, d_d)));
                auto s_new = hn::IfThenElse(erode, hn::Add(s, d_e), hn::Sub(s, d_d));

                // aplikuj tylko gdzie mokro
                hn::StoreU(hn::IfThenElse(wet, ti_new, ti), d_tag, ter + i);
                hn::StoreU(hn::IfThenElse(wet, s_new,  s),  d_tag, sediment.data() + i);
            }

            for (; x < size - 1; x++) {
                scalar(x);
            }

            // prawa kolumna
            scalar_border(size - 1, y);
        }
    }
}


// FAZA 4: transport sedymentu
// sediment_new jest pre-alokowany, brak alokacji per-iteracja
void PipeErosionOptimized::step_sediment_transport(int size, const Ref<PipeErosionSettings>& cfg) {
    const float dt = cfg->get_dt();
    const float l = cfg->get_pipe_length();
    const float size_max = float(size - 1) - 1e-4f;

    // zerowanie bufora, bo std::fill jest SIMD-friendly, kompilator wektoryzuje
    std::fill(sediment_new.begin(), sediment_new.end(), 0.0f);

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            const int i = y * size + x;

            float px = std::max(0.0f, std::min(size_max, float(x) - vel_x[i] * dt / l));
            float py = std::max(0.0f, std::min(size_max, float(y) - vel_y[i] * dt / l));
            
            int x0 = int(px), y0 = int(py);
            int x1 = std::min(x0 + 1,size - 1), y1=std::min(y0 + 1,size - 1);
            float fx = px - float(x0), fy = py - float(y0);

            sediment_new[i] = std::max(0.0f,
                sediment[y0 * size + x0] * (1 - fx) * (1 - fy)
                + sediment[y0 * size + x1] * fx * (1 - fy)
                + sediment[y1 * size + x0] * (1 - fx) * fy
                + sediment[y1 * size + x1] * fx * fy);
        }
    }

    std::swap(sediment, sediment_new);
}


// FAZA 5: parowanie
void PipeErosionOptimized::step_evaporation(std::vector<float>& terrain, int size, const Ref<PipeErosionSettings>& cfg) {
    const float factor = 1.0f - cfg->get_Ke() * cfg->get_dt();
    const int n = size * size;

    const hn::ScalableTag<float> d_tag;
    const int W = static_cast<int>(hn::Lanes(d_tag));

    const auto v_factor = hn::Set(d_tag, factor);
    const auto v_zero = hn::Zero(d_tag);
    const auto v_eps = hn::Set(d_tag, 1e-5f);

    const int n_blocks = n / W;

    // OpenMP po blokach W, każdy wątek dostaje zakres bloków, nie pojedynczych elementów
    #pragma omp parallel for schedule(static)
    for (int b = 0; b < n_blocks; b++) {
        const int i = b * W;
        auto w = hn::Max(v_zero, hn::Mul(hn::LoadU(d_tag, water.data() + i), v_factor));
        auto s = hn::LoadU(d_tag, sediment.data() + i);

        // gdzie woda != 0
        auto below = hn::Lt(w, v_eps);
        auto t = hn::LoadU(d_tag, terrain.data() + i);

        // tam gdzie poniżej progu: terrain += sediment
        t = hn::Add(t, hn::IfThenElseZero(below, s));
        
        // sediment = 0 tam, else s
        s = hn::IfThenZeroElse(below, s);
        hn::StoreU(t, d_tag, terrain.data() + i);

        hn::StoreU(w, d_tag, water.data() + i);
        hn::StoreU(s, d_tag, sediment.data() + i);
    }

    // ogon - elementy które nie zmieściły się w pełnym bloku W
    for (int i = n_blocks * W; i < n; i++) {
        water[i] = std::max(0.0f, water[i] * factor);
        if (water[i] < 1e-5f){
            terrain[i] += sediment[i];
            sediment[i] = 0.0f;
        }
    }
}



// ERODE
void PipeErosionOptimized::erode(Ref<Heightmap> heightmap, int num_iterations, const Ref<PipeErosionSettings>& config) {
    if (heightmap.is_null() || heightmap->is_empty()) {
        ERR_PRINT("PipeErosionOptimized::erode - heightmap is null or empty");
        return;
    }

    const int size = heightmap->size;
    alloc(size);

    std::vector<float> terrain(heightmap->data.ptr(), heightmap->data.ptr() + size * size);

    const int rain_every = std::max(1, config->get_rain_iterations());
    const float rain_dt = config->get_rain_rate() * config->get_dt();

    for (int iter = 0; iter < num_iterations; iter++) {
        if (iter % rain_every == 0) {
            // SIMD rain - Highway na płaskiej tablicy
            const hn::ScalableTag<float> d_tag;
            const int W = static_cast<int>(hn::Lanes(d_tag));
            const auto v_rain = hn::Set(d_tag, rain_dt);
            const int n = size * size;
            const int n_blocks = n / W;

            #pragma omp parallel for schedule(static)
            for (int b = 0; b < n_blocks; b++) {
                const int i = b * W;
                hn::StoreU(hn::Add(hn::LoadU(d_tag, water.data() + i), v_rain), d_tag, water.data() + i);
            }
            for (int i = n_blocks * W; i < n; i++)
                water[i] += rain_dt;
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