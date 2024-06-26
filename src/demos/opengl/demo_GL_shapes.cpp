// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Hammad Mazhar
// =============================================================================
//
// Chrono::OpenGL test program.
//
// A Random Set of Geometries in Space
// The global reference frame has Z up.
// =============================================================================

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#endif

#include "chrono/physics/ChSystemNSC.h"
#include "chrono/utils/ChUtilsCreators.h"

#include "chrono_opengl/ChVisualSystemOpenGL.h"

using namespace chrono;

int main(int argc, char* argv[]) {
    std::cout << "Copyright (c) 2017 projectchrono.org\nChrono version: " << CHRONO_VERSION << std::endl;

    ChSystemNSC sys;
    auto mat = chrono_types::make_shared<ChContactMaterialNSC>();
    auto bin = chrono_types::make_shared<ChBody>();
    sys.AddBody(bin);

    double a = 0.5;
    double b = 0.25;
    double c = 0.1;
    ChVector3d xdir(1.5, 0.0, 0.0);
    ChVector3d ydir(0.0, 1.5, 0.0);
    ChVector3d zdir(0.0, 0.0, 1.5);
    ChQuaternion<> rot(1, 0, 0, 0);
    rot = QuatFromAngleX(CH_PI / 6);

    utils::AddSphereGeometry(bin.get(), mat, 0.05, ChVector3d(0, 0, 0));

    utils::AddSphereGeometry(bin.get(), mat, a, xdir * 1, rot);
    utils::AddSphereGeometry(bin.get(), mat, b, ydir * 1, rot);
    utils::AddSphereGeometry(bin.get(), mat, c, zdir * 1, rot);

    utils::AddEllipsoidGeometry(bin.get(), mat, ChVector3d(2 * a, 4 * a, 4 * a), xdir * 2, rot);
    utils::AddEllipsoidGeometry(bin.get(), mat, ChVector3d(4 * b, 2 * b, 4 * b), ydir * 2, rot);
    utils::AddEllipsoidGeometry(bin.get(), mat, ChVector3d(4 * c, 4 * c, 2 * c), zdir * 2, rot);

    utils::AddBoxGeometry(bin.get(), mat, ChVector3d(2 * a, 4 * a, 4 * a), xdir * 3, rot);
    utils::AddBoxGeometry(bin.get(), mat, ChVector3d(4 * b, 2 * b, 4 * b), ydir * 3, rot);
    utils::AddBoxGeometry(bin.get(), mat, ChVector3d(4 * c, 4 * c, 2 * c), zdir * 3, rot);

    utils::AddCylinderGeometry(bin.get(), mat, a, 0.5, xdir * 4, rot);
    utils::AddCylinderGeometry(bin.get(), mat, b, 0.5, ydir * 4, rot);
    utils::AddCylinderGeometry(bin.get(), mat, c, 0.5, zdir * 4, rot);

    utils::AddConeGeometry(bin.get(), mat, a, 1.5, xdir * 5, rot);
    utils::AddConeGeometry(bin.get(), mat, b, 1.5, ydir * 5, rot);
    utils::AddConeGeometry(bin.get(), mat, c, 1.5, zdir * 5, rot);

    utils::AddCapsuleGeometry(bin.get(), mat, a, 1.0, xdir * 6, rot);
    utils::AddCapsuleGeometry(bin.get(), mat, b, 1.0, ydir * 6, rot);
    utils::AddCapsuleGeometry(bin.get(), mat, c, 1.0, zdir * 6, rot);

    // Render everything
    opengl::ChVisualSystemOpenGL vis;
    vis.AttachSystem(&sys);
    vis.SetWindowTitle("OpenGL Shapes");
    vis.SetWindowSize(1280, 720);
    vis.SetRenderMode(opengl::WIREFRAME);
    vis.Initialize();
    vis.AddCamera(ChVector3d(6, -10, 0), ChVector3d(6, 0, 0));
    vis.SetCameraVertical(CameraVerticalDir::Z);

    std::function<void()> step_iter = [&]() { vis.Render(); };

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(&opengl::ChVisualSystemOpenGL::WrapRenderStep, (void*)&step_iter, 50, true);
#else
    while (vis.Run()) {
        step_iter();
    }
#endif

    return 0;
}
