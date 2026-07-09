#include "Heightmap.hpp"

using namespace godot;



void Heightmap::_bind_methods() {
    ClassDB::bind_method(D_METHOD("smooth", "iterations"), &Heightmap::smooth);
    ClassDB::bind_method(D_METHOD("get_data"), &Heightmap::get_data);
    ClassDB::bind_method(D_METHOD("get_size"), &Heightmap::get_size);
    ClassDB::bind_method(D_METHOD("is_empty"), &Heightmap::is_empty);
    ClassDB::bind_method(D_METHOD("clear"), &Heightmap::clear);
    ClassDB::bind_method(D_METHOD("load_from_data", "data", "size"), &Heightmap::load_from_data);
}



bool Heightmap::is_empty() const { 
    return data.is_empty(); 
}

void Heightmap::clear() { 
    data.clear(); 
    size = 0; 
}
void Heightmap::from_vec(const std::vector<std::vector<float>>& vec) {
    size = static_cast<int>(vec.size());
    data.resize(size * size);
    for (int y = 0; y < size; y++){
        for (int x = 0; x < size; x++){
            data[y * size + x] = vec[y][x];
        }
    }
}

void Heightmap::smooth(int iterations) {
    if (is_empty() || size <= 1) 
        return;

    // operates on PackedFloat32Array instead of converting to std::vector and back in each   
    PackedFloat32Array copy;

    for (int iter = 0; iter < iterations; iter++) {
        copy = data;

        const float* src = copy.ptr();
        float* dst = data.ptrw();

        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                int x0 = (x > 0) ? x-1 : 1;
                int x1 = (x < size-1) ? x+1 : size-2;
                int y0 = (y > 0) ? y-1 : 1;
                int y1 = (y < size - 1) ? y+1 : size-2;

                dst[y*size + x] = (src[y*size + x0] + src[y*size + x1] + src[y0*size + x] + src[y1*size + x] + src[y*size + x]) / 5.0f;
            }
        }
    }
}

PackedFloat32Array Heightmap::get_data() const { 
    return data; 
}

int Heightmap::get_size() const { 
    return size; 
}

void Heightmap::load_from_data(PackedFloat32Array new_data, int new_size) {
    data = new_data;
    size = new_size;
}