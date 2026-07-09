#pragma once

#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "../resources/DiamondSquareSettings.hpp"
#include "../Heightmap.hpp"

using namespace godot;



class DiamondSquare : public RefCounted {
    GDCLASS(DiamondSquare, RefCounted);

protected:
    static void _bind_methods();

private:
    static void square_step(std::vector<std::vector<float>>& vec, int step_size, int half_step, Vector2i random);
    static void diamond_step(std::vector<std::vector<float>>& vec, int step_size, int half_step, Vector2i random);
    static int correct_map_size(int size);

public:
    DiamondSquare() = default;

    Ref<Heightmap> generate(const Ref<DiamondSquareSettings>& config);
};
