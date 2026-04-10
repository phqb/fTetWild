// This file is part of fTetWild, a software for generating tetrahedral meshes.
//
// Copyright (C) 2019 Yixin Hu <yixin.hu@nyu.edu>
// This Source Code Form is subject to the terms of the Mozilla Public License
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.
//

#pragma once

// C interface for floatTetWild::run.
// Usable from C, Python (ctypes/cffi), Rust (FFI), etc.
//
// Workflow:
//   FTW_TetMesh* m = ftw_run(verts, nv, tris, nt, 0.05, 0, 0);
//   // query ftw_get_num_vertices / ftw_get_num_tetrahedra, then fill buffers
//   ftw_free(m);
//
// All arrays are flat / row-major:
//   in_vertices    – doubles [x0,y0,z0, x1,y1,z1, …], length = 3 * num_vertices
//   in_triangles   – ints    [v0,v1,v2, …],            length = 3 * num_triangles
//   out_vertices   – doubles [x0,y0,z0, …],            length = 3 * ftw_get_num_vertices(m)
//   out_tetrahedra – ints    [a,b,c,d, …],             length = 4 * ftw_get_num_tetrahedra(m)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FTW_TetMesh FTW_TetMesh;

// Run tetrahedralization.  Returns NULL on failure.
FTW_TetMesh* ftw_run(const double* in_vertices,  int num_vertices,
                     const int*    in_triangles, int num_triangles,
                     double lr, unsigned int max_threads,
                     int skip_simplify);

int  ftw_tet_mesh_get_num_vertices(const FTW_TetMesh* mesh);
int  ftw_tet_mesh_get_num_tetrahedra(const FTW_TetMesh* mesh);

// Copy results into caller-supplied buffers.
void ftw_tet_mesh_get_vertices(const FTW_TetMesh* mesh, double* out_vertices);
void ftw_tet_mesh_get_tetrahedra(const FTW_TetMesh* mesh, int* out_tetrahedra);

void ftw_tet_mesh_free(FTW_TetMesh* mesh);

// Set the spdlog log level (use spdlog::level::level_enum integer values).
void ftw_set_log_level(int level);

#ifdef __cplusplus
} // extern "C"
#endif
