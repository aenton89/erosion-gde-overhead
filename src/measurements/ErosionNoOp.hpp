#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include "../generation/Heightmap.hpp"
#include "../erosion/resources/PipeErosionSettings.hpp"

using namespace godot;



// do pomiaru samego kosztu przekroczenia granicy GDS - GDE - mierzony czas to niemal wyłącznie narzut wywołania
class ErosionNoOp : public RefCounted {
    GDCLASS(ErosionNoOp, RefCounted);

protected:
    static void _bind_methods();

public:
    ErosionNoOp() = default;

    // czysty narzut wywołania - punkt odniesienia
    void noop() {}

    // narzut marshallingu pojedynczego prymitywu
    void noop_int(int v) {}
    void noop_float(float v) {}

    // narzut transferu masowego
    void noop_array(PackedFloat32Array arr) {}
    int64_t noop_array_size(PackedFloat32Array arr) { return arr.size(); }

    // sygnatura PipeErosion::erode
    void noop_mockup(Ref<Heightmap> heightmap, int num_iterations, const Ref<PipeErosionSettings>& config) {}
};