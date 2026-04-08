// This file is part of fTetWild, a software for generating tetrahedral meshes.
//
// Copyright (C) 2019 Yixin Hu <yixin.hu@nyu.edu>
// This Source Code Form is subject to the terms of the Mozilla Public License
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.
//

#include "Run.h"

#include <floattetwild/AABBWrapper.h>
#include <floattetwild/FloatTetDelaunay.h>
#include <floattetwild/LocalOperations.h>
#include <floattetwild/Mesh.hpp>
#include <floattetwild/MeshImprovement.h>
#include <floattetwild/MeshIO.hpp>
#include <floattetwild/Logger.hpp>
#include <floattetwild/Simplification.h>
#include <floattetwild/Statistics.h>
#include <floattetwild/TriangleInsertion.h>
#include <floattetwild/Types.hpp>

#include <geogram/basic/command_line.h>
#include <geogram/basic/command_line_args.h>
#include <geogram/basic/common.h>
#include <geogram/basic/logger.h>
#include <geogram/mesh/mesh.h>

#ifdef FLOAT_TETWILD_USE_TBB
#include <oneapi/tbb.h>
#include <oneapi/tbb/global_control.h>
#include <igl/default_num_threads.h>
#include <thread>
#endif

#include <map>
#include <mutex>

namespace floatTetWild {

namespace {

std::once_flag geo_init_flag;
std::once_flag logger_init_flag;

} // namespace

TetMesh run(const std::vector<std::array<double, 3>>& in_vertices,
            const std::vector<std::array<int, 3>>&    in_triangles,
            double       lr,
            unsigned int max_threads,
            bool         skip_simplify,
            int          log_level)
{
    // Initialize geogram and logger exactly once across all calls
    std::call_once(geo_init_flag, []() {
#ifndef WIN32
        setenv("GEO_NO_SIGNAL_HANDLER", "1", 1);
#endif
        GEO::initialize();
        GEO::CmdLine::import_arg_group("standard");
        GEO::CmdLine::import_arg_group("pre");
        GEO::CmdLine::import_arg_group("algo");
    });
    std::call_once(logger_init_flag, []() {
        Logger::init(true, "");
        spdlog::flush_every(std::chrono::seconds(3));
        // Suppress geogram's own stdout output; we use spdlog via Logger
        GEO::Logger::instance()->unregister_all_clients();
        GEO::Logger::instance()->set_pretty(false);
    });

    Mesh        mesh;
    Parameters& params = mesh.params;

    params.ideal_edge_length_rel = lr;
    params.log_level             = std::max(0, std::min(6, log_level));

#ifdef FLOAT_TETWILD_USE_TBB
    const unsigned int hw_threads  = std::max(1u, std::thread::hardware_concurrency());
    const unsigned int num_threads = (max_threads == 0)
                                         ? hw_threads
                                         : std::min(max_threads, hw_threads);
    params.num_threads             = num_threads;
    const size_t MB                = 1024 * 1024;
    const size_t stack_size        = 64 * MB;
    tbb::global_control parallelism_limit(
        tbb::global_control::max_allowed_parallelism, num_threads);
    tbb::global_control stack_size_limit(
        tbb::global_control::thread_stack_size, stack_size);
    // Avoid oversubscription in IGL's nested parallel loops, see
    // https://github.com/libigl/libigl/issues/2412
    igl::default_num_threads(std::ceil(std::sqrt(num_threads)));
#endif

    spdlog::set_level(static_cast<spdlog::level::level_enum>(params.log_level));

    // Convert input to internal representation
    std::vector<Vector3>  input_vertices(in_vertices.size());
    std::vector<Vector3i> input_faces(in_triangles.size());

    for (size_t i = 0; i < in_vertices.size(); ++i)
        input_vertices[i] << in_vertices[i][0], in_vertices[i][1], in_vertices[i][2];

    for (size_t i = 0; i < in_triangles.size(); ++i)
        input_faces[i] << in_triangles[i][0], in_triangles[i][1], in_triangles[i][2];

    if (input_vertices.empty() || input_faces.empty())
        return {};

    // Build GEO::Mesh for AABB (also Morton-reorders input_vertices/input_faces)
    GEO::Mesh        sf_mesh;
    std::vector<int> input_tags(input_faces.size(), 0);
    MeshIO::load_mesh(input_vertices, input_faces, sf_mesh, input_tags);

    AABBWrapper tree(sf_mesh);
#ifdef NEW_ENVELOPE
    tree.init_sf_tree(input_vertices, input_faces, params.eps);
#endif
    if (!params.init(tree.get_sf_diag()))
        return {};

    stats().record(StateInfo::init_id, 0, input_vertices.size(), input_faces.size(), -1, -1);

    // Step 1: Preprocessing (skipped via skip_simplify=true)
    simplify(input_vertices, input_faces, input_tags, tree, params, skip_simplify);
    tree.init_b_mesh_and_tree(input_vertices, input_faces, mesh);

    // Step 2: Volume tetrahedralization
    std::vector<bool> is_face_inserted(input_faces.size(), false);
    FloatTetDelaunay::tetrahedralize(input_vertices, input_faces, tree, mesh, is_face_inserted);

    // Step 3: Triangle insertion
    insert_triangles(input_vertices, input_faces, input_tags, mesh, is_face_inserted, tree, false);

    // Step 4: Mesh optimization
    optimization(
        input_vertices, input_faces, input_tags, is_face_inserted, mesh, tree, {{1, 1, 1, 1}});

    // Step 5: Interior extraction
    correct_tracked_surface_orientation(mesh, tree);
    filter_outside(mesh);

    // Extract tetrahedra following the .mesh format output logic from MeshIO.cpp:
    // vertices are enumerated in natural order, tetrahedra use index permutation {0,1,3,2}
    // to match the .mesh convention, with 0-based indices.
    int                cnt_v = 0;
    std::map<int, int> old_2_new;
    for (int i = 0; i < (int)mesh.tet_vertices.size(); i++) {
        if (!mesh.tet_vertices[i].is_removed) {
            old_2_new[i] = cnt_v;
            cnt_v++;
        }
    }

    TetMesh result;
    result.vertices.reserve(cnt_v);
    for (int i = 0; i < (int)mesh.tet_vertices.size(); i++) {
        if (mesh.tet_vertices[i].is_removed)
            continue;
        result.vertices.push_back(
            {mesh.tet_vertices[i][0], mesh.tet_vertices[i][1], mesh.tet_vertices[i][2]});
    }

    const std::array<int, 4> new_indices = {{0, 1, 3, 2}};
    for (int i = 0; i < (int)mesh.tets.size(); i++) {
        if (mesh.tets[i].is_removed)
            continue;
        Tetrahedron tet;
        for (int j = 0; j < 4; j++)
            tet.v[j] = old_2_new[mesh.tets[i][new_indices[j]]];
        result.tetrahedra.push_back(tet);
    }

    return result;
}

} // namespace floatTetWild
