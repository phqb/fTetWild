// This file is part of fTetWild, a software for generating tetrahedral meshes.
//
// Copyright (C) 2019 Yixin Hu <yixin.hu@nyu.edu>
// This Source Code Form is subject to the terms of the Mozilla Public License
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.
//

#include "RunC.h"
#include "Run.h"

#include <spdlog/spdlog.h>

#include <array>
#include <vector>

struct FTW_TetMesh {
    floatTetWild::TetMesh mesh;
};

extern "C" {

FTW_TetMesh* ftw_run(const double* in_vertices,  int num_vertices,
                     const int*    in_triangles, int num_triangles,
                     double lr, unsigned int max_threads,
                     int skip_simplify)
{
    std::vector<std::array<double, 3>> verts(num_vertices);
    for (int i = 0; i < num_vertices; ++i)
        verts[i] = {in_vertices[3*i], in_vertices[3*i+1], in_vertices[3*i+2]};

    std::vector<std::array<int, 3>> tris(num_triangles);
    for (int i = 0; i < num_triangles; ++i)
        tris[i] = {in_triangles[3*i], in_triangles[3*i+1], in_triangles[3*i+2]};

    auto* result = new FTW_TetMesh;
    result->mesh = floatTetWild::run(verts, tris, lr, max_threads,
                                     skip_simplify != 0);
    return result;
}

int ftw_tet_mesh_get_num_vertices(const FTW_TetMesh* mesh)
{
    return static_cast<int>(mesh->mesh.vertices.size());
}

int ftw_tet_mesh_get_num_tetrahedra(const FTW_TetMesh* mesh)
{
    return static_cast<int>(mesh->mesh.tetrahedra.size());
}

void ftw_tet_mesh_get_vertices(const FTW_TetMesh* mesh, double* out)
{
    const auto& verts = mesh->mesh.vertices;
    for (int i = 0; i < (int)verts.size(); ++i) {
        out[3*i]   = verts[i].x;
        out[3*i+1] = verts[i].y;
        out[3*i+2] = verts[i].z;
    }
}

void ftw_tet_mesh_get_tetrahedra(const FTW_TetMesh* mesh, int* out)
{
    const auto& tets = mesh->mesh.tetrahedra;
    for (int i = 0; i < (int)tets.size(); ++i) {
        out[4*i]   = tets[i].v[0];
        out[4*i+1] = tets[i].v[1];
        out[4*i+2] = tets[i].v[2];
        out[4*i+3] = tets[i].v[3];
    }
}

void ftw_tet_mesh_free(FTW_TetMesh* mesh)
{
    delete mesh;
}

void ftw_set_log_level(int level)
{
    spdlog::set_level(static_cast<spdlog::level::level_enum>(level));
}

} // extern "C"
