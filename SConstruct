# #!/usr/bin/env python
# SConscript("godot-cpp/SConstruct")
# CacheDir(".scons_cache/")

#!/usr/bin/env python
from glob import glob
import os



env = SConscript("godot-cpp/SConstruct")
env.Append(CPPPATH=["src/", "."])

# for OpenMP and SIMD
if env["platform"] == "windows":
    env.Append(CCFLAGS=["/openmp", "/std:c++17"])
    # highway downloaded by vcpkg:
    env.Append(CPPPATH=["C:/vcpkg/installed/x64-windows/include"])


sources = glob("src/**/*.cpp", recursive=True)
# DEPRECATED: all documentation is comented out
# doc_sources = glob("doc_classes/**/*.xml", recursive=True)


# generates doc_data if supported
# if env["target"] in ["editor", "template_release"]:
#     try:
#         doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=doc_sources)
#         sources.append(doc_data)
#     except AttributeError:
#         print("Not including class reference as targeting a pre-4.2 baseline.")



# here are all generated gdextension files
folder = "build/addons/Erosion"
# submodule (Godot example project)
project_folder = "godot_project/addons/Erosion"



# build library 
if env["platform"] == "macos":
	file_name = "libErosion.{}.{}".format(env["platform"], env["target"])

	library = env.SharedLibrary(
		"{}/{}.framework/{}".format(folder, file_name, file_name),
		source=sources
	)
else:
	library = env.SharedLibrary(
		"{}/libErosion{}{}"
			.format(folder, env["suffix"], env["SHLIBSUFFIX"]),
		source=sources,
	)




# copy .gdextension to build folder
gdextension_copy = env.Command(
	target="{}/Erosion.gdextension".format(folder),
	source="Erosion.gdextension",
	action=Copy("$TARGET", "$SOURCE")
)

env.Depends(gdextension_copy, library)

# copy .gdextension to submodule
gdextension_copy_example = env.Command(
    target="{}/Erosion.gdextension".format(project_folder),
    source="Erosion.gdextension",
    action=Copy("$TARGET", "$SOURCE")
)
env.Depends(gdextension_copy_example, library)


# copy built library (+ side files) to submodule 
def copy_files_action(target, source, env):
    for s, t in zip(source, target):
        env.Execute(Copy(str(t), str(s)))
    return 0

if env["platform"] == "windows":
    dll_path = str(library[0])
    lib_path = dll_path.replace(".dll", ".lib")
    exp_path = dll_path.replace(".dll", ".exp")

    lib_copy_example = env.Command(
        target=[
            os.path.join(project_folder, os.path.basename(dll_path)),
            os.path.join(project_folder, os.path.basename(lib_path)),
            os.path.join(project_folder, os.path.basename(exp_path)),
        ],
        source=[dll_path, lib_path, exp_path],
        action=copy_files_action
    )
elif env["platform"] == "macos":
    dylib_path = str(library[0])
    lib_copy_example = env.Command(
        target=os.path.join(project_folder, os.path.basename(dylib_path)),
        source=dylib_path,
        action=copy_files_action
    )
else:  # linux
    so_path = str(library[0])
    lib_copy_example = env.Command(
        target=os.path.join(project_folder, os.path.basename(so_path)),
        source=so_path,
        action=copy_files_action
    )

env.Depends(lib_copy_example, library)

# copy doc_classes to submodule
# def copy_dir_action(target, source, env):
#     import shutil
#     import os
#     src_dir = str(source[0])
#     dst_dir = str(target[0])

#     if os.path.exists(dst_dir):
#         shutil.rmtree(dst_dir)
#     shutil.copytree(src_dir, dst_dir)
#     return 0

# doc_classes_copy_example = env.Command(
#     target=os.path.join(project_folder, "doc_classes"),
#     source="doc_classes",
#     action=copy_dir_action
# )
# env.Depends(doc_classes_copy_example, doc_sources)



CacheDir(".scons_cache/")

Default(library)
Default(gdextension_copy)
Default(gdextension_copy_example)
Default(lib_copy_example)
# Default(doc_classes_copy_example)