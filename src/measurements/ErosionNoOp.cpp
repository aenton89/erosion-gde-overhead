#include "ErosionNoOp.hpp"



void ErosionNoOp::_bind_methods() {
    ClassDB::bind_method(D_METHOD("noop"), &ErosionNoOp::noop);
    ClassDB::bind_method(D_METHOD("noop_int", "v"), &ErosionNoOp::noop_int);
    ClassDB::bind_method(D_METHOD("noop_float", "v"), &ErosionNoOp::noop_float);
    ClassDB::bind_method(D_METHOD("noop_array", "arr"), &ErosionNoOp::noop_array);
    ClassDB::bind_method(D_METHOD("noop_array_size", "arr"), &ErosionNoOp::noop_array_size);
    ClassDB::bind_method(D_METHOD("noop_mockup", "heightmap", "num_iterations", "config"), &ErosionNoOp::noop_mockup);
}