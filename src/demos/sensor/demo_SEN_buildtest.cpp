// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2019 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Asher Elmquist
// =============================================================================
//
// Chrono demonstration for testing the chrono sensor module
//
//
// =============================================================================

#include <cmath>
#include <cstdio>
#include <iomanip>

#include "chrono/assets/ChTriangleMeshShape.h"
#include "chrono/assets/ChVisualMaterial.h"
#include "chrono/assets/ChVisualization.h"
#include "chrono/geometry/ChTriangleMeshConnected.h"
#include "chrono/physics/ChBodyEasy.h"
#include "chrono/physics/ChSystemNSC.h"
#include "chrono/physics/ChInertiaUtils.h"
#include "chrono/utils/ChUtilsCreators.h"
#include "chrono_thirdparty/filesystem/path.h"

#include "chrono_sensor/ChCameraSensor.h"
#include "chrono_sensor/ChGPSSensor.h"
#include "chrono_sensor/ChIMUSensor.h"
#include "chrono_sensor/ChNoiseModel.h"
#include "chrono_sensor/ChLidarSensor.h"
#include "chrono_sensor/ChSensorManager.h"
#include "chrono_sensor/filters/ChFilterAccess.h"
#include "chrono_sensor/filters/ChFilterImageOps.h"
#include "chrono_sensor/filters/ChFilterLidarIntensityClip.h"
#include "chrono_sensor/filters/ChFilterCameraNoise.h"
#include "chrono_sensor/filters/ChFilterGrayscale.h"
#include "chrono_sensor/filters/ChFilterPCfromDepth.h"
#include "chrono_sensor/filters/ChFilterSave.h"
#include "chrono_sensor/filters/ChFilterSavePtCloud.h"
#include "chrono_sensor/filters/ChFilterVisualize.h"
#include "chrono_sensor/filters/ChFilterVisualizePointCloud.h"

using namespace chrono;
using namespace chrono::geometry;
using namespace chrono::sensor;

int num_cameras = 2 - 1;
int num_bodies = 10;
int num_groups = 1;

bool save_data = false;
bool display_data = true;

bool run_chrono = true;
float time_step = 0.002f;
float end_time = 30.0f;

int main(int argc, char* argv[]) {
    GetLog() << "Copyright (c) 2019 projectchrono.org\nChrono version: " << CHRONO_VERSION << "\n\n";

    // -----------------
    // Create the system
    // -----------------
    auto phys_mat = chrono_types::make_shared<ChMaterialSurfaceNSC>();
    phys_mat->SetFriction(0.2f);

    ChSystemNSC mphysicalSystem;
    mphysicalSystem.Set_G_acc({0, 0, -9.81});

    auto floor = chrono_types::make_shared<ChBodyEasyBox>(100, 100, 1,      // x,y,z size
                                                          1000,             // density
                                                          true,             // collide enable?
                                                          true, phys_mat);  // visualization?
    floor->SetPos({0, 0, -1.0});
    floor->SetRot(Q_from_AngZ(CH_C_PI / 2.0));
    floor->SetBodyFixed(true);
    mphysicalSystem.Add(floor);

    // place objects to visually test rotations
    auto scalebox = chrono_types::make_shared<ChBodyEasyBox>(1, .2, .4,        // x,y,z size
                                                             1000,             // density
                                                             true,             // collide enable?
                                                             true, phys_mat);  // visualization?
    scalebox->SetPos({0, -1, 1});
    scalebox->SetBodyFixed(true);
    mphysicalSystem.Add(scalebox);

    // test max reflections
    auto top_mirror = chrono_types::make_shared<ChBodyEasyBox>(10, 10, .1,       // x,y,z size
                                                               1000,             // density
                                                               true,             // collide enable?
                                                               true, phys_mat);  // visualization?
    top_mirror->SetPos({0, -1, 1.5});
    top_mirror->SetBodyFixed(true);
    mphysicalSystem.Add(top_mirror);
    auto top_m_asset = top_mirror->GetAssets()[0];
    if (std::shared_ptr<ChVisualization> visual_asset = std::dynamic_pointer_cast<ChVisualization>(top_m_asset)) {
        auto vis_mat = chrono_types::make_shared<ChVisualMaterial>();
        // vis_mat->SetAmbientColor({0.f, 0.f, 0.f});
        vis_mat->SetDiffuseColor({1, 1, 1});
        // vis_mat->SetAmbientColor(.2 * vis_mat->GetDiffuseColor());
        vis_mat->SetSpecularColor({1.f, 1.f, 1.f});
        vis_mat->SetRoughness(0);
        visual_asset->material_list.push_back(vis_mat);
    }

    auto bottom_mirror = chrono_types::make_shared<ChBodyEasyBox>(10, 10, .1,       // x,y,z size
                                                                  1000,             // density
                                                                  true,             // collide enable?
                                                                  true, phys_mat);  // visualization?
    bottom_mirror->SetPos({0, -1, 0.5});
    bottom_mirror->SetBodyFixed(true);
    mphysicalSystem.Add(bottom_mirror);
    auto bottom_m_asset = bottom_mirror->GetAssets()[0];
    if (std::shared_ptr<ChVisualization> visual_asset = std::dynamic_pointer_cast<ChVisualization>(bottom_m_asset)) {
        auto vis_mat = chrono_types::make_shared<ChVisualMaterial>();
        // vis_mat->SetAmbientColor({0.f, 0.f, 0.f});
        vis_mat->SetDiffuseColor({1, 1, 1});
        // vis_mat->SetAmbientColor(.2 * vis_mat->GetDiffuseColor());
        vis_mat->SetSpecularColor({1.f, 1.f, 1.f});
        vis_mat->SetRoughness(0);
        visual_asset->material_list.push_back(vis_mat);
    }
    // add a mesh
    auto mmesh = chrono_types::make_shared<ChTriangleMeshConnected>();
    mmesh->LoadWavefrontMesh(GetChronoDataFile("models/bulldozer/shoe_collision.obj"), false, true);
    mmesh->Transform(ChVector<>(0, 0, 0), ChMatrix33<>(1));  // scale to a different size
    mmesh->RepairDuplicateVertexes(1e-9);

    double mmass;
    ChVector<> mcog;
    ChMatrix33<> minertia;
    double mdensity = 1000;
    mmesh->ComputeMassProperties(true, mmass, mcog, minertia);
    ChMatrix33<> principal_inertia_rot;
    ChVector<> principal_I;
    ChInertiaUtils::PrincipalInertia(minertia, principal_I, principal_inertia_rot);
    auto trimesh_shape = chrono_types::make_shared<ChTriangleMeshShape>();
    trimesh_shape->SetMesh(mmesh);
    trimesh_shape->SetName("Mesh");
    trimesh_shape->SetStatic(true);

    std::shared_ptr<ChBody> imu_parent;
    std::shared_ptr<ChBody> gps_parent;

    // float size = 3;
    // auto origin = chrono_types::make_shared<ChBodyEasyBox>(size, size, size, 1000, true, false);
    // origin->SetPos({0, 0, 1});
    // origin->SetBodyFixed(true);
    //
    // mphysicalSystem.Add(origin);
    // auto origin_asset = origin->GetAssets()[0];
    // if (std::shared_ptr<ChVisualization> v_asset = std::dynamic_pointer_cast<ChVisualization>(origin_asset)) {
    //     auto vis_mat = chrono_types::make_shared<ChVisualMaterial>();
    //     vis_mat->SetDiffuseColor({1, 0, 0});
    //     vis_mat->SetSpecularColor({.5f, .5f, .5f});
    //
    //     v_asset->material_list.push_back(vis_mat);
    // }

    // walls to contain falling objects
    auto wall1 = chrono_types::make_shared<ChBodyEasyBox>(40.0, .1, 10.0, 1000, true, true, phys_mat);
    wall1->SetPos({0, -20, 4});
    wall1->SetBodyFixed(true);
    mphysicalSystem.Add(wall1);

    auto wall2 = chrono_types::make_shared<ChBodyEasyBox>(40.0, .1, 10.0, 1000, true, true, phys_mat);
    wall2->SetPos({0, 20, 4});
    wall2->SetBodyFixed(true);
    mphysicalSystem.Add(wall2);

    auto wall3 = chrono_types::make_shared<ChBodyEasyBox>(.1, 40.0, 10.0, 1000, true, true, phys_mat);
    wall3->SetPos({-20, 0, 4});
    wall3->SetBodyFixed(true);
    mphysicalSystem.Add(wall3);

    auto wall4 = chrono_types::make_shared<ChBodyEasyBox>(.1, 40.0, 10.0, 1000, true, true, phys_mat);
    wall4->SetPos({20, 0, 4});
    wall4->SetBodyFixed(true);
    mphysicalSystem.Add(wall4);

    // add box, sphere, mesh with textures
    // auto gator_mesh = chrono_types::make_shared<ChTriangleMeshConnected>();
    // gator_mesh->LoadWavefrontMesh(GetChronoDataFile("vehicle/gator/gator_chassis.obj"), false, true);
    // auto gator_meshshape = chrono_types::make_shared<ChTriangleMeshShape>();
    // gator_meshshape->SetMesh(gator_mesh);
    // gator_meshshape->SetName("gator");
    // gator_meshshape->SetStatic(true);
    // auto gator_mesh_body = chrono_types::make_shared<ChBody>();
    // gator_mesh_body->SetPos({1, 2, 2});
    // gator_mesh_body->AddAsset(gator_meshshape);
    // gator_mesh_body->SetBodyFixed(true);
    // mphysicalSystem.Add(gator_mesh_body);

    auto texbox = chrono_types::make_shared<ChBodyEasyBox>(1, 1,
                                                           1,                 // x,y,z size
                                                           1000,              // density
                                                           true,              // vis enable?
                                                           false, phys_mat);  //
    texbox->SetPos({1, 0, 3});
    texbox->SetBodyFixed(true);
    mphysicalSystem.Add(texbox);

    auto texbox_asset = texbox->GetAssets()[0];
    if (std::shared_ptr<ChVisualization> visual_asset = std::dynamic_pointer_cast<ChVisualization>(texbox_asset)) {
        auto vis_mat = chrono_types::make_shared<ChVisualMaterial>();
        vis_mat->SetSpecularColor({.2f, .2f, .2f});
        vis_mat->SetKdTexture(GetChronoDataFile("textures/redwhite.png"));
        vis_mat->SetNormalMapTexture(GetChronoDataFile("sensor/textures/FaceNormal.jpg"));
        visual_asset->material_list.push_back(vis_mat);
    }

    auto texsphere = chrono_types::make_shared<ChBodyEasySphere>(.6,                // size
                                                                 1000,              // density
                                                                 true,              // vis enable?
                                                                 false, phys_mat);  //
    texsphere->SetPos({1, -2, 3});
    texsphere->SetBodyFixed(true);
    mphysicalSystem.Add(texsphere);

    auto texsphere_asset = texsphere->GetAssets()[0];
    if (std::shared_ptr<ChVisualization> visual_asset = std::dynamic_pointer_cast<ChVisualization>(texsphere_asset)) {
        auto vis_mat = chrono_types::make_shared<ChVisualMaterial>();
        vis_mat->SetSpecularColor({.2f, .2f, .2f});
        vis_mat->SetKdTexture(GetChronoDataFile("textures/redwhite.png"));
        vis_mat->SetNormalMapTexture(GetChronoDataFile("sensor/textures/FaceNormal.jpg"));
        visual_asset->material_list.push_back(vis_mat);
    }

    auto texcyl = chrono_types::make_shared<ChBodyEasyCylinder>(.5, 1,             // size
                                                                1000,              // density
                                                                true,              // vis enable?
                                                                false, phys_mat);  //
    texcyl->SetPos({1, -4, 3});
    texcyl->SetBodyFixed(true);
    mphysicalSystem.Add(texcyl);

    auto texcyl_asset = texcyl->GetAssets()[0];
    if (std::shared_ptr<ChVisualization> visual_asset = std::dynamic_pointer_cast<ChVisualization>(texcyl_asset)) {
        auto vis_mat = chrono_types::make_shared<ChVisualMaterial>();
        vis_mat->SetSpecularColor({.2f, .2f, .2f});
        vis_mat->SetKdTexture(GetChronoDataFile("textures/redwhite.png"));
        vis_mat->SetNormalMapTexture(GetChronoDataFile("sensor/textures/FaceNormal.jpg"));
        visual_asset->material_list.push_back(vis_mat);
    }

    for (int i = 0; i < num_bodies; i++) {
        // add a box
        auto box =
            chrono_types::make_shared<ChBodyEasyBox>((float)ChRandom() / 2.0 + 0.1, (float)ChRandom() / 2.0 + 0.1,
                                                     (float)ChRandom() / 2.0 + 0.1,  // x,y,z size
                                                     1000,                           // density
                                                     true,                           // collide enable?
                                                     true, phys_mat);                // visualization?
        box->SetPos({(float)ChRandom(), (float)ChRandom(), 2.0 + i});
        box->SetRot(Q_from_Euler123({(float)ChRandom(), (float)ChRandom(), (float)ChRandom()}));
        mphysicalSystem.Add(box);

        auto box_asset = box->GetAssets()[0];
        if (std::shared_ptr<ChVisualization> visual_asset = std::dynamic_pointer_cast<ChVisualization>(box_asset)) {
            auto vis_mat = chrono_types::make_shared<ChVisualMaterial>();
            // vis_mat->SetAmbientColor({0.f, 0.f, 0.f});
            vis_mat->SetDiffuseColor({(float)ChRandom(), (float)ChRandom(), (float)ChRandom()});
            // vis_mat->SetAmbientColor(.2 * vis_mat->GetDiffuseColor());
            vis_mat->SetSpecularColor({.2f, .2f, .2f});

            visual_asset->material_list.push_back(vis_mat);
        }

        if (!imu_parent) {
            imu_parent = box;
        }

        auto cyl = chrono_types::make_shared<ChBodyEasyCylinder>((float)ChRandom() / 2.0 + 0.1,  // radius
                                                                 (float)ChRandom() / 2.0 + 0.1,  // height
                                                                 1000,                           // density
                                                                 true,                           // collide enable?
                                                                 true, phys_mat);                // visualization?
        cyl->SetPos({(float)ChRandom(), (float)ChRandom(), 2.0 + i});
        cyl->SetRot(Q_from_Euler123({(float)ChRandom(), (float)ChRandom(), (float)ChRandom()}));
        mphysicalSystem.Add(cyl);

        auto cyl_asset = cyl->GetAssets()[0];
        if (std::shared_ptr<ChVisualization> visual_asset = std::dynamic_pointer_cast<ChVisualization>(cyl_asset)) {
            auto vis_mat = chrono_types::make_shared<ChVisualMaterial>();
            // vis_mat->SetAmbientColor({0.f, 0.f, 0.f});
            vis_mat->SetDiffuseColor({(float)ChRandom(), (float)ChRandom(), (float)ChRandom()});
            // vis_mat->SetAmbientColor(.2 * vis_mat->GetDiffuseColor());
            vis_mat->SetSpecularColor({.2f, .2f, .2f});

            visual_asset->material_list.push_back(vis_mat);
        }

        auto sphere = chrono_types::make_shared<ChBodyEasySphere>((float)ChRandom() / 2.0 + 0.1,  // radius
                                                                  1000,                           // density
                                                                  true,                           // collide enable?
                                                                  true, phys_mat);                // visualization?
        sphere->SetPos({(float)ChRandom(), (float)ChRandom(), 2.0 + i});
        // sphere->SetRot(Q_from_Euler123({(float)ChRandom(), (float)ChRandom(), (float)rand() /
        // RAND_MAX}));
        mphysicalSystem.Add(sphere);
        if (!gps_parent) {
            gps_parent = sphere;
        }

        auto sphere_asset = sphere->GetAssets()[0];
        if (std::shared_ptr<ChVisualization> visual_asset = std::dynamic_pointer_cast<ChVisualization>(sphere_asset)) {
            auto vis_mat = chrono_types::make_shared<ChVisualMaterial>();
            vis_mat->SetAmbientColor({0.f, 0.f, 0.f});
            vis_mat->SetDiffuseColor({(float)ChRandom(), (float)ChRandom(), (float)ChRandom()});
            vis_mat->SetSpecularColor({.2f, .2f, .2f});

            visual_asset->material_list.push_back(vis_mat);
        }

        // auto mesh_body = chrono_types::make_shared<ChBody>();
        // mesh_body->SetPos({0, 0, 0});
        // mesh_body->AddAsset(trimesh_shape);
        // mesh_body->SetBodyFixed(true);
        // mesh_body->GetCollisionModel()->ClearModel();
        // mesh_body->GetCollisionModel()->AddTriangleMesh(phys_mat, mmesh, true, false);
        // mesh_body->GetCollisionModel()->BuildModel();
        // mphysicalSystem.Add(mesh_body);

        auto mesh_body = chrono_types::make_shared<ChBodyAuxRef>();
        mesh_body->SetFrame_COG_to_REF(ChFrame<>(mcog, principal_inertia_rot));
        mesh_body->SetMass(mmass * mdensity);
        mesh_body->SetInertiaXX(mdensity * principal_I);
        mesh_body->SetFrame_REF_to_abs(ChFrame<>(ChVector<>((float)ChRandom(), (float)ChRandom(), 2.0 + i)));
        mphysicalSystem.Add(mesh_body);

        mesh_body->GetCollisionModel()->ClearModel();
        mesh_body->GetCollisionModel()->AddTriangleMesh(phys_mat, mmesh, false, false, VNULL, ChMatrix33<>(1), 0.005);
        mesh_body->GetCollisionModel()->BuildModel();
        mesh_body->SetCollide(true);

        mesh_body->AddAsset(trimesh_shape);
    }

    std::cout << "sensor manager being made\n";
    auto manager = chrono_types::make_shared<ChSensorManager>(&mphysicalSystem);
    manager->SetDeviceList({2, 1, 0});
    manager->SetMaxEngines(num_groups);  // THIS NEEDS MORE PERFORMANCE TESTING
    manager->SetVerbose(false);

    // make some changes/additions to the scene
    PointLight p0 = {{10, 10, 10}, {1, 1, 1}, 1000};
    PointLight p1 = {{10, 10, 10}, {0, 0, 1}, 1000};
    manager->scene->AddPointLight(p0);
    manager->scene->AddPointLight(p1);
    manager->scene->AddPointLight({0, 0, 100}, {1, 1, 1}, 1000);

    // set the background
    Background b;
    b.mode = BackgroundMode::GRADIENT;
    b.color_zenith = {.5f, .6f, .7f};
    b.color_horizon = {.9f, .8f, .7f};
    // b.env_tex = GetChronoDataFile("sensor/textures/sky_2_4k.hdr");
    manager->scene->SetBackground(b);

    auto cam = chrono_types::make_shared<ChCameraSensor>(
        floor,                                                               // body camera is attached to
        60.0f,                                                               // update rate in Hz
        chrono::ChFrame<double>({-10, 0, 1}, Q_from_AngAxis(0, {0, 0, 1})),  // offset pose
        1920,                                                                // image width
        1080,                                                                // image height
        (float)CH_C_PI / 4,                                                  // field of view
        2, CameraLensModelType::FOV_LENS, false);

    std::string color_data_path = "SENSOR_OUTPUT/cam_color/";
    std::string gray_data_path = "SENSOR_OUTPUT/cam_gray/";
    std::string lidar_data_path = "SENSOR_OUTPUT/lidar/";

    cam->SetName("Camera Sensor 0");
    // cam->SetLag();
    // cam->SetCollectionWindow(imu_collection_time);

    // cam->PushFilter(chrono_types::make_shared<ChFilterCameraNoiseConstNormal>(0.f, 0.01));
    // cam->PushFilter(chrono_types::make_shared<ChFilterCameraNoisePixDep>(0.01f, 0.01f, 0.01f));

    // we want to save the RGBA buffer to png
    if (save_data)
        cam->PushFilter(chrono_types::make_shared<ChFilterSave>(color_data_path));

    // we want to visualize this sensor right after rendering, so add the visualize filter to the filter list.
    // if (display_data)
    cam->PushFilter(chrono_types::make_shared<ChFilterVisualize>(1280, 720, "Whitted, before Grayscale Filter"));

    // we want to have access to this RGBA8 buffer on the host.
    cam->PushFilter(chrono_types::make_shared<ChFilterRGBA8Access>());

    cam->PushFilter(chrono_types::make_shared<ChFilterImageResize>(1280, 720, "resize filter"));

    // // filter the sensor to grayscale
    cam->PushFilter(chrono_types::make_shared<ChFilterGrayscale>());
    //
    // // we want to visualize this sensor after grayscale, so add the visualize filter to the filter list.
    if (display_data)
        cam->PushFilter(chrono_types::make_shared<ChFilterVisualize>(1280, 720, "Whitted, Final Visualization"));
    //
    // // we want to save the grayscale buffer to png
    if (save_data)
        cam->PushFilter(chrono_types::make_shared<ChFilterSave>(gray_data_path));
    //
    // // we also want to have access to this grayscale buffer on the host.
    cam->PushFilter(chrono_types::make_shared<ChFilterR8Access>());

    // add sensor to the manager
    manager->AddSensor(cam);

    // add a lidar to the floor facing the falling objects
    auto lidar = chrono_types::make_shared<ChLidarSensor>(
        floor,                                                              // body to which the IMU is attached
        10.0f,                                                              // update rate
        chrono::ChFrame<double>({-8, 0, 1}, Q_from_AngAxis(0, {1, 0, 0})),  // offset pose from body
        923,                                                                // horizontal samples
        23,                                                                 // vertical samples/channels
        2.f * (float)CH_C_PI / 3.0f,                                        // horizontal field of view
        (float)CH_C_PI / 8.0f, -(float)CH_C_PI / 8.0f, 100.0f               // vertical field of view
    );
    lidar->SetName("Lidar Sensor");
    lidar->SetLag(.1f);
    lidar->SetCollectionWindow(.1f);

    // lidar->PushFilter(chrono_types::make_shared<ChFilterLidarIntensityClip>(.02f, 0.f, "Intensity Clip Filter"));

    if (display_data)
        lidar->PushFilter(chrono_types::make_shared<ChFilterVisualize>(923, 48, "Raw Lidar Data"));
    lidar->PushFilter(chrono_types::make_shared<ChFilterDIAccess>());
    lidar->PushFilter(chrono_types::make_shared<ChFilterPCfromDepth>());
    if (display_data)
        lidar->PushFilter(chrono_types::make_shared<ChFilterVisualizePointCloud>(640, 480, 2.0f, "Lidar Point Cloud"));
    if (save_data)
        lidar->PushFilter(chrono_types::make_shared<ChFilterSavePtCloud>(lidar_data_path));
    lidar->PushFilter(chrono_types::make_shared<ChFilterXYZIAccess>());
    // lidar->PushFilter(chrono_types::make_shared<ChFilterXYZIAccess>());
    // manager->AddSensor(lidar);

    // add a lidar to the floor facing the falling objects
    auto lidar2 = chrono_types::make_shared<ChLidarSensor>(
        floor,                                                              // body to which the IMU is attached
        10.0f,                                                              // update rate
        chrono::ChFrame<double>({-8, 0, 1}, Q_from_AngAxis(0, {1, 0, 0})),  // offset pose from body
        923,                                                                // horizontal samples
        23,                                                                 // vertical samples/channels
        2.f * (float)CH_C_PI / 3.0f,                                        // horizontal field of view
        (float)CH_C_PI / 8.0f,                                              // max vert angle
        -(float)CH_C_PI / 8.0f,                                             // min vert angle
        100.0f,                                                             // max range
        LidarBeamShape::RECTANGULAR,                                        // beam shape
        3,                                                                  // beam sample radius
        .003,                                                               // vert divergence angle
        .003,                                                               // horizontal divergence angle
        LidarReturnMode::STRONGEST_RETURN                                   // return type
    );
    lidar2->SetName("Lidar Sensor 2");
    lidar2->SetLag(.1f);
    lidar2->SetCollectionWindow(.1f);

    if (display_data)
        lidar2->PushFilter(chrono_types::make_shared<ChFilterVisualize>(923, 48, "Raw Lidar Data"));
    lidar2->PushFilter(chrono_types::make_shared<ChFilterDIAccess>());
    lidar2->PushFilter(chrono_types::make_shared<ChFilterPCfromDepth>());
    if (display_data)
        lidar2->PushFilter(
            chrono_types::make_shared<ChFilterVisualizePointCloud>(640, 480, 2.0f, "Lidar2 Point Cloud"));
    lidar2->PushFilter(chrono_types::make_shared<ChFilterXYZIAccess>());
    // manager->AddSensor(lidar2);

    // add an IMU sensor to one of the boxes
    auto imu_offset_pose = chrono::ChFrame<double>({0, 0, 0}, Q_from_AngAxis(0, {1, 0, 0}));
    auto noise_none = chrono_types::make_shared<ChNoiseNone>();
    auto acc = chrono_types::make_shared<ChAccelerometerSensor>(imu_parent,       // body to which the IMU is
                                                                                  // attached
                                                                100,              // update rate
                                                                imu_offset_pose,  // offset pose from body
                                                                noise_none);      // IMU noise model
    acc->SetName("IMU - Accelerometer");
    acc->PushFilter(chrono_types::make_shared<ChFilterAccelAccess>());  // Add a filter to access the imu data
    manager->AddSensor(acc);                                            // Add the IMU sensor to the sensor manager

    auto gyro = chrono_types::make_shared<ChGyroscopeSensor>(imu_parent,       // body to which the IMU is attached
                                                             100,              // update rate
                                                             imu_offset_pose,  // offset pose from body
                                                             noise_none);      // IMU noise model
    gyro->SetName("IMU - Accelerometer");
    gyro->PushFilter(chrono_types::make_shared<ChFilterGyroAccess>());  // Add a filter to access the imu data
    manager->AddSensor(gyro);                                           // Add the IMU sensor to the sensor manager

    auto mag = chrono_types::make_shared<ChMagnetometerSensor>(imu_parent,       // body to which the IMU is attached
                                                               100,              // update rate
                                                               imu_offset_pose,  // offset pose from body
                                                               noise_none,       // IMU noise model
                                                               ChVector<double>(43.300, -89.000, 260.0));
    mag->SetName("IMU - Accelerometer");
    mag->PushFilter(chrono_types::make_shared<ChFilterMagnetAccess>());  // Add a filter to access the imu data
    manager->AddSensor(mag);                                             // Add the IMU sensor to the sensor manager

    // add an IMU sensor to one of the boxes
    auto noise_model =
        chrono_types::make_shared<ChNoiseNormal>(ChVector<float>(0.f, 0.f, 0.f), ChVector<float>(1.f, 1.f, 1.f));
    auto gps = chrono_types::make_shared<ChGPSSensor>(
        gps_parent,                                                        // body to which the GPS is attached
        10,                                                                // update rate
        chrono::ChFrame<double>({0, 0, 0}, Q_from_AngAxis(0, {1, 0, 0})),  // offset pose from body
        ChVector<double>(43.300, -89.000, 260.0),  // reference GPS location (GPS coordinates of simulation origin)
        noise_model                                // noise model to use for adding GPS noise (NOT THREAD SAFE)
    );
    gps->SetName("GPS");
    gps->PushFilter(chrono_types::make_shared<ChFilterGPSAccess>());
    manager->AddSensor(gps);

    std::vector<std::shared_ptr<ChCameraSensor>> cams;

    for (int i = 0; i < num_cameras; i++) {
        auto cam1 = chrono_types::make_shared<ChCameraSensor>(
            floor,                                                              // body camera is attached to
            10.0f + 10.0f * (i % 4 + 1),                                        // 30 + i, // update rate in Hz
            chrono::ChFrame<double>({-3, 0, 2}, Q_from_AngAxis(0, {1, 0, 0})),  // offset pose
            1280,                                                               // image width
            720,                                                                // image height
            (float)CH_C_PI / 3);
        cams.push_back(cam1);

        std::stringstream nm;
        nm << "Camera Sensor " << i + 1;
        cam1->SetName(nm.str());
        cam1->SetLag(.1f);
        cam1->SetCollectionWindow(0);

        // we want to visualize this sensor, so add the visualize filter to the filter list.
        if (display_data)
            cam1->PushFilter(chrono_types::make_shared<ChFilterVisualize>(1280, 720, "Before Grayscale Filter"));

        // filter the sensor to grayscale
        cam1->PushFilter(chrono_types::make_shared<ChFilterGrayscale>());

        // we want to visualize this sensor after grayscale, so add the visualize filter to the filter list.
        if (display_data)
            cam1->PushFilter(chrono_types::make_shared<ChFilterVisualize>(1280, 720, "After Grayscale Filter"));

        if (save_data)
            cam1->PushFilter(chrono_types::make_shared<ChFilterSave>("SENSOR_OUTPUT/cam" + std::to_string(i) + "/"));

        // add sensor to the manager
        manager->AddSensor(cam1);
    }

    // manager->ReconstructScenes();
    // ---------------
    // Simulate system
    // ---------------
    float orbit_radius = 15.f;
    float orbit_rate = .2f;
    float ch_time = 0.f;

    double render_time = 0;

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

    float light_change = -.002f;

    std::cout << "Sensor Mangager has: " << manager->GetNumEngines() << " engines running.\n";
    while (ch_time < end_time) {
        std::chrono::high_resolution_clock::time_point r0 = std::chrono::high_resolution_clock::now();
        cam->SetOffsetPose(chrono::ChFrame<double>(
            {-orbit_radius * cos(ch_time * orbit_rate), -orbit_radius * sin(ch_time * orbit_rate), 3},
            Q_from_AngAxis(ch_time * orbit_rate, {0, 0, 1})));

        scalebox->SetRot(Q_from_AngY(ch_time * .3));

        p0.pos = {-orbit_radius * cos(ch_time * orbit_rate * 2), -orbit_radius * sin(ch_time * orbit_rate * 2), 10};
        manager->scene->ModifyPointLight(0, p0);

        p1.color = make_float3(p1.color.x, p1.color.y, p1.color.z + light_change);
        if (p1.color.z < 0) {
            p1.color = make_float3(0, 0, 0);
            light_change = -light_change;
        }
        if (p1.color.z > 1) {
            p1.color = make_float3(0, 0, 1);
            light_change = -light_change;
        }
        manager->scene->ModifyPointLight(1, p1);

        // origin->SetRot(Q_from_AngAxis(ch_time * orbit_rate, {0, 0, 1}));
        // origin->SetPos({0, 0, 3 * sin(ch_time * orbit_rate)});

        manager->Update();
        std::chrono::high_resolution_clock::time_point r1 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> t_render = std::chrono::duration_cast<std::chrono::duration<double>>(r1 - r0);
        render_time += t_render.count();

        mphysicalSystem.DoStepDynamics(time_step);

        ch_time = (float)mphysicalSystem.GetChTime();
    }
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> wall_time = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
    std::cout << "Simulation time: " << ch_time << "s, wall time: " << wall_time.count() << "s.\n";
    std::cout << "Time spent in Chrono: " << wall_time.count() - render_time
              << ", extra time due to rendering: " << render_time << std::endl;

    return 0;
}
