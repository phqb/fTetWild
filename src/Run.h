// This file is part of fTetWild, a software for generating tetrahedral meshes.
//
// Copyright (C) 2019 Yixin Hu <yixin.hu@nyu.edu>
// This Source Code Form is subject to the terms of the Mozilla Public License
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.
//

#pragma once

#include <array>
#include <vector>

namespace floatTetWild {

struct TetVertex {
    double x, y, z;
};

struct Tetrahedron {
    std::array<int, 4> v; // 0-based vertex indices
};

struct TetMesh {
    std::vector<TetVertex>   vertices;
    std::vector<Tetrahedron> tetrahedra;
};

// Run FloatTetwild tetrahedralization.
//
// Input:
//   in_vertices   - triangle mesh vertices, each as {x, y, z}
//   in_triangles  - triangle face indices (0-based), each as {v0, v1, v2}
//   lr            - ideal edge length as a fraction of the bounding box diagonal (--lr)
//   max_threads   - maximum number of TBB threads; 0 = hardware concurrency (--max-threads)
//   skip_simplify - skip input mesh preprocessing/simplification (--skip-simplify)
//
// Output:
//   TetMesh with vertices and tetrahedra (0-based indices, vertex order
//   matching the .mesh format: internal indices reordered as {0,1,3,2})
TetMesh run(const std::vector<std::array<double, 3>>& in_vertices,
            const std::vector<std::array<int, 3>>&    in_triangles,
            double       lr            = 0.05,
            unsigned int max_threads   = 0,
            bool         skip_simplify = false);

} // namespace floatTetWild
