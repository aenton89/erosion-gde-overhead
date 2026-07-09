#include "TerrainRenderer3D.hpp"

using namespace godot;



void TerrainRenderer3D::_bind_methods() {
    ClassDB::bind_method(D_METHOD("generate_mesh", "heightmap"), &TerrainRenderer3D::generate_mesh);
    ClassDB::bind_method(D_METHOD("set_mesh_on_node", "heightmap", "mesh_instance"), &TerrainRenderer3D::set_mesh_on_node);

    ClassDB::bind_method(D_METHOD("set_height_scale", "v"), &TerrainRenderer3D::set_height_scale);
    ClassDB::bind_method(D_METHOD("get_height_scale"), &TerrainRenderer3D::get_height_scale);
    ClassDB::bind_method(D_METHOD("set_cell_size", "v"), &TerrainRenderer3D::set_cell_size);
    ClassDB::bind_method(D_METHOD("get_cell_size"), &TerrainRenderer3D::get_cell_size);

    ADD_GROUP("Mesh Settings", "");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_scale"), "set_height_scale", "get_height_scale");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cell_size"), "set_cell_size", "get_cell_size");
}



Ref<ArrayMesh> TerrainRenderer3D::generate_mesh(Ref<Heightmap> heightmap) {
    if (heightmap.is_null() || heightmap->is_empty()) {
        ERR_PRINT("Heightmap is empty.");
        return {};
    }

    const int size = heightmap->size;
    // unique vertices
    const int n_verts = size * size;
    // 2 triangles * 3 indices
    const int n_indices = (size - 1) * (size - 1) * 6;
    const float inv_size1 = 1.0f / float(size - 1);

    const float* hdata = heightmap->data.ptr();

    auto get_h = [&](int x, int y) -> float {
        return float(hdata[y * size + x]) * height_scale;
    };

    // 1. VERTICES, UV, colors (size*size of uniques)
    PackedVector3Array verts; 
    verts.resize(n_verts);
    PackedVector2Array uvs; 
    uvs.resize(n_verts);
    {
        Vector3* vp = verts.ptrw();
        Vector2* up = uvs.ptrw();
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                int i = y * size + x;
                vp[i] = Vector3(x * cell_size, get_h(x, y), y * cell_size);
                up[i] = Vector2(float(x) * inv_size1, float(y) * inv_size1);
            }
        }
    }

    // 2. INDICES
    PackedInt32Array indices;
    indices.resize(n_indices);
    {
        int* ip = indices.ptrw();
        int  ii = 0;
        for (int y = 0; y < size - 1; y++) {
            for (int x = 0; x < size - 1; x++) {
                int i00 = y * size + x;
                int i10 = y * size + x + 1;
                int i01 = (y + 1) * size + x;
                int i11 = (y + 1) * size + x + 1;

                // triangle 1
                ip[ii++] = i00; 
                ip[ii++] = i10; 
                ip[ii++] = i11;
                // triangle 2
                ip[ii++] = i00; 
                ip[ii++] = i11; 
                ip[ii++] = i01;
            }
        }
    }

    // 3. NORMALS - rewrite vertices through indices and use SurfaceTool::generate_normals() (for lighting), should have small cost?
    {
        Ref<SurfaceTool> st;  
        st.instantiate();
        st->begin(Mesh::PRIMITIVE_TRIANGLES);

        const Vector3* vp = verts.ptr();
        const Vector2* up = uvs.ptr();
        const int* ip = indices.ptr();

        for (int i = 0; i < n_indices; i++) {
            int idx = ip[i];
            st->set_uv(up[idx]);
            st->add_vertex(vp[idx]);
        }

        st->generate_normals();

        Ref<ArrayMesh> mesh;
        mesh.instantiate();
        mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, st->commit_to_arrays());
        return mesh;
    }
}

void TerrainRenderer3D::set_mesh_on_node(Ref<Heightmap> heightmap, MeshInstance3D* mesh_instance) {
    if (!mesh_instance) { 
        ERR_PRINT("MeshInstance3D is null."); 
        return; 
    }

    Ref<ArrayMesh> mesh = generate_mesh(heightmap);
    if (mesh.is_valid())
        mesh_instance->set_mesh(mesh);
}

void TerrainRenderer3D::set_height_scale(float v) { 
    height_scale = v; 
}

float TerrainRenderer3D::get_height_scale() const { 
    return height_scale; 
}

void TerrainRenderer3D::set_cell_size(float v) { 
    cell_size = v; 
}

float TerrainRenderer3D::get_cell_size() const { 
    return cell_size; 
}