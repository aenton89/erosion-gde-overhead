#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "generation/Heightmap.hpp"
#include "generation/algorithms/PerlinNoise.hpp"
#include "generation/resources/PerlinNoiseSettings.hpp"
#include "generation/algorithms/DiamondSquare.hpp"
#include "generation/resources/DiamondSquareSettings.hpp"

#include "representations/TerrainRenderer2D.hpp"
#include "representations/TerrainRenderer3D.hpp"

#include "measurements/TimeMemoryProfiler.h"
#include "measurements/ErosionNoOp.hpp"

#include "erosion/algorithms/HydraulicErosion.hpp"
#include "erosion/resources/HydraulicErosionSettings.hpp"
#include "erosion/algorithms/PipeErosion.hpp"
#include "erosion/resources/PipeErosionSettings.hpp"
#include "erosion/algorithms/PipeErosionOptimized.hpp"

#include "register_types.hpp"

using namespace godot;



// registers static classes, components etc.
void initialize_erosion(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
        return;

    GDREGISTER_CLASS(Heightmap);
    GDREGISTER_ABSTRACT_CLASS(GenerationSettings);
    GDREGISTER_CLASS(DiamondSquareSettings);
    GDREGISTER_CLASS(PerlinNoiseSettings);
    GDREGISTER_CLASS(DiamondSquare);
    GDREGISTER_CLASS(PerlinNoise);

    GDREGISTER_CLASS(TerrainRenderer2D);
    GDREGISTER_CLASS(TerrainRenderer3D);

    GDREGISTER_CLASS(HydraulicErosion);
    GDREGISTER_CLASS(HydraulicErosionSettings);
    GDREGISTER_CLASS(PipeErosion);
    GDREGISTER_CLASS(PipeErosionSettings);
    GDREGISTER_CLASS(PipeErosionOptimized);


    GDREGISTER_CLASS(ErosionNoOp);
    GDREGISTER_CLASS(ProfilerGD);
    Engine::get_singleton()->register_singleton("Profiler", memnew(ProfilerGD));
}

// and cleanup here, like memory etc.
void uninitialize_erosion(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE){
        if (Engine::get_singleton()->has_singleton("Profiler")) {
            memdelete(Engine::get_singleton()->get_singleton("Profiler"));
            Engine::get_singleton()->unregister_singleton("Profiler");
        }
    }
}

extern "C" {
    // connects plugin to godot
    auto GDE_EXPORT erosion_entry(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) -> GDExtensionBool {
        godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_erosion);
        init_obj.register_terminator(uninitialize_erosion);

        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}