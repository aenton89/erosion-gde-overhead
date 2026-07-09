#include "DiamondSquare.hpp"
#include <random>

using namespace godot;



void DiamondSquare::_bind_methods() {
    ClassDB::bind_method(D_METHOD("generate", "config"), &DiamondSquare::generate);
}



int DiamondSquare::correct_map_size(int size) {
    if (size <= 3)
        return 3;
    if (size <= 5)   
        return 5;
    if (size <= 9)   
        return 9;
    if (size <= 17)  
        return 17;
    if (size <= 33)  
        return 33;
    if (size <= 65)  
        return 65;
    if (size <= 129) 
        return 129;
    if (size <= 257) 
        return 257;
    if (size <= 513) 
        return 513;
    return 1025;
}

void DiamondSquare::square_step(std::vector<std::vector<float>>& vec, int step_size, int half_step, Vector2i random) {
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> dist(random.x, random.y);

    int n = vec.size();
    for (int y = 0; y < n-1; y += step_size) {
        for (int x = 0; x < n-1; x += step_size) {
            float avg = (vec[y][x] + vec[y][x+step_size] + vec[y+step_size][x] + vec[y+step_size][x+step_size]) / 4;
            vec[y+half_step][x+half_step] = avg + dist(gen);
        }
    }
}

void DiamondSquare::diamond_step(std::vector<std::vector<float>>& vec, int step_size, int half_step, Vector2i random) {
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> dist(random.x, random.y);

    int n = vec.size();
    for (int y = 0; y < n; y += half_step) {
        for (int x = (y+half_step) % step_size; x < n; x += step_size) {
            float sum = 0, count = 0;
            if (y-half_step >= 0) { 
                sum += vec[y-half_step][x]; 
                count++; 
            }
            if (x+half_step < n)  { 
                sum += vec[y][x+half_step]; 
                count++; 
            }
            if (y+half_step < n)  { 
                sum += vec[y+half_step][x]; 
                count++; 
            }
            if (x-half_step >= 0) { 
                sum += vec[y][x-half_step]; 
                count++; 
            }
            vec[y][x] = (sum/count) + dist(gen);
        }
    }
}

Ref<Heightmap> DiamondSquare::generate(const Ref<DiamondSquareSettings>& config) {
    int size = config->get_map_size();
    Vector2i heights = config->get_height_range();
    Vector2i random = config->get_random();
    int decrease_factor = config->get_decrease_factor();

    if (size < 2 || ((size-1) & (size-2)) != 0) {
        ERR_PRINT("size must be 2^n + 1 and at least 2");
        return {};
    }

    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(heights.x, heights.y);

    std::vector<std::vector<float>> vec(size, std::vector<float>(size));
    vec[0][0] = dist(gen);
    vec[0][size-1] = dist(gen);
    vec[size-1][0] = dist(gen);
    vec[size-1][size-1] = dist(gen);

    int step_size = size - 1;
    Vector2i cur_random = random;

    while (step_size > 1) {
        int half_step = step_size / 2;
        square_step(vec, step_size, half_step, cur_random);
        diamond_step(vec, step_size, half_step, cur_random);

        step_size /= 2;
        if (decrease_factor > 0) {
            cur_random.x -= decrease_factor;
            cur_random.y -= decrease_factor;
        } else {
            cur_random.x /= 2;
            cur_random.y /= 2;
        }
    }

    Ref<Heightmap> heightmap;
    heightmap.instantiate();
    heightmap->from_vec(vec);
    return heightmap;
}