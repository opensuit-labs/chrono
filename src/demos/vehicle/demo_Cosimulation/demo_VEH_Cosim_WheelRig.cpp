// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2020 projectchrono.org
// All right reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Radu Serban
// =============================================================================
//
// Demo for single-wheel rig cosimulation framework
//
// Global reference frame: Z up, X towards the front, and Y pointing to the left
//
// =============================================================================

#include <iostream>
#include <string>
#include <mpi.h>

#include "chrono/ChConfig.h"

#include "chrono_vehicle/ChVehicleModelData.h"

#include "chrono_thirdparty/cxxopts/ChCLI.h"
#include "chrono_thirdparty/filesystem/path.h"

#include "chrono_vehicle/cosim/ChVehicleCosimRigNode.h"
#include "chrono_vehicle/cosim/ChVehicleCosimTerrainNodeSCM.h"
#ifdef CHRONO_MULTICORE
    #include "chrono_vehicle/cosim/ChVehicleCosimTerrainNodeRigid.h"
    #include "chrono_vehicle/cosim/ChVehicleCosimTerrainNodeGranularOMP.h"
#endif
#ifdef CHRONO_FSI
    #include "chrono_vehicle/cosim/ChVehicleCosimTerrainNodeGranularSPH.h"
#endif
#ifdef CHRONO_GPU
    #include "chrono_vehicle/cosim/ChVehicleCosimTerrainNodeGranularGPU.h"
#endif

using std::cout;
using std::cin;
using std::endl;

using namespace chrono;
using namespace chrono::vehicle;

// =============================================================================


// Tire type
ChVehicleCosimRigNode::Type tire_type = ChVehicleCosimRigNode::Type::RIGID;

// =============================================================================

// Forward declarations
bool GetProblemSpecs(int argc,
                     char** argv,
                     int rank,
                     std::string& terrain_specfile,
                     int& nthreads_rig,
                     int& nthreads_terrain,
                     double& step_size,
                     double& settling_time,
                     double& sim_time,
                     double& vel0,
                     double& slip,
                     double& sys_mass,
                     double& terrain_length,
                     double& terrain_width,
                     bool& use_checkpoint,
                     double& output_fps,
                     double& render_fps,
                     bool& sim_output,
                     bool& settling_output,
                     bool& render,
                     bool& verbose,
                     std::string& suffix);

// =============================================================================

int main(int argc, char** argv) {
    // Initialize MPI.
    int num_procs;
    int rank;
    int name_len;
    char procname[MPI_MAX_PROCESSOR_NAME];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(procname, &name_len);

#ifdef _DEBUG
    if (rank == 0) {
        int foo;
        cout << "Enter something to continue..." << endl;
        cin >> foo;
    }
    MPI_Barrier(MPI_COMM_WORLD);
#endif

    if (num_procs != 2) {
        if (rank == 0)
            std::cout << "\n\nSingle wheel cosimulation code must be run on exactly 2 ranks!\n\n" << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    // Parse command line arguments
    std::string terrain_specfile;
    int nthreads_rig = 1;
    int nthreads_terrain = 1;
    double step_size = 1e-4;
    double settling_time = 0.4;
    double sim_time = 10;
    double vel0 = 0.5;
    double slip = 0;
    bool use_checkpoint = false;
    double output_fps = 100;
    double render_fps = 100;
    bool sim_output = true;
    bool settling_output = true;
    bool render = true;
    double sys_mass = 200;
    double terrain_length = 4;
    double terrain_width = 1;
    std::string suffix = "";
    bool verbose = true;
    if (!GetProblemSpecs(argc, argv, rank, terrain_specfile, nthreads_rig, nthreads_terrain, step_size, settling_time,
                         sim_time, vel0, slip, sys_mass, terrain_length, terrain_width, use_checkpoint, output_fps,
                         render_fps, sim_output, settling_output, render, verbose, suffix)) {
        MPI_Finalize();
        return 1;
    }

    // Peek in spec file and extract terrain type
    auto terrain_type = ChVehicleCosimTerrainNode::GetTypeFromSpecfile(terrain_specfile);
    if (terrain_type == ChVehicleCosimTerrainNode::Type::UNKNOWN) {
        MPI_Finalize();
        return 1;
    }

// Check if required modules are enabled
#ifndef CHRONO_MULTICORE
    if (terrain_type == ChVehicleCosimTerrainNode::Type::RIGID ||
        terrain_type == ChVehicleCosimTerrainNode::Type::GRANULAR_OMP) {
        if (rank == 0)
            cout << "Chrono::Multicore is required for RIGID or GRANULAR_OMP terrain type!" << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
#endif
#ifndef CHRONO_GPU
    if (terrain_type == ChVehicleCosimTerrainNode::Type::GRANULAR_GPU) {
        if (rank == 0)
            cout << "Chrono::Gpu is required for GRANULAR_GPU terrain type!" << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
#endif
#ifndef CHRONO_FSI
    if (terrain_type == ChVehicleCosimTerrainNode::Type::GRANULAR_SPH) {
        if (rank == 0)
            cout << "Chrono::FSI is required for GRANULAR_SPH terrain type!" << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
#endif

    // Prepare output directory.
    std::string out_dir_top = GetChronoOutputPath() + "RIG_COSIM";
    std::string out_dir = out_dir_top + "/" +                                        //
                          ChVehicleCosimRigNode::GetTypeAsString(tire_type) + "_" +  //
                          ChVehicleCosimTerrainNode::GetTypeAsString(terrain_type);
    if (rank == 0) {
        if (!filesystem::create_directory(filesystem::path(out_dir_top))) {
            cout << "Error creating directory " << out_dir_top << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
        if (!filesystem::create_directory(filesystem::path(out_dir))) {
            cout << "Error creating directory " << out_dir << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // Number of simulation steps between miscellaneous events.
    int sim_steps = (int)std::ceil(sim_time / step_size);
    int output_steps = (int)std::ceil(1 / (output_fps * step_size));

    // Create the node (a rig node or a terrain node, depending on rank).
    ChVehicleCosimBaseNode* node = nullptr;

    if (rank == RIG_NODE_RANK) {
        if (verbose)
            cout << "[Rig node    ] rank = " << rank << " running on: " << procname << endl;

        switch (tire_type) {
            case ChVehicleCosimRigNode::Type::RIGID: {
                auto rig = new ChVehicleCosimRigNodeRigidTire(vel0, slip, nthreads_rig);
                rig->SetVerbose(verbose);
                rig->SetTireJSONFile(vehicle::GetDataFile("hmmwv/tire/HMMWV_RigidMeshTire_CoarseClosed.json"));
                rig->SetBodyMasses(1, 1, sys_mass, 15);
                rig->SetDBPfilterWindow(0.2);
                node = rig;
                break;
            }
            case ChVehicleCosimRigNode::Type::FLEXIBLE: {
                auto rig = new ChVehicleCosimRigNodeFlexibleTire(vel0, slip, nthreads_rig);
                rig->SetVerbose(verbose);
                rig->SetTireJSONFile(vehicle::GetDataFile("hmmwv/tire/HMMWV_ANCFTire.json"));
                rig->SetBodyMasses(1, 1, sys_mass, 15);
                rig->EnableTirePressure(true);
                rig->SetDBPfilterWindow(0.2);
                node = rig;
                break;
            }
        }

        node->SetStepSize(step_size);
        node->SetOutDir(out_dir, suffix);

        if (verbose)
            cout << "[Rig node    ] output directory: " << node->GetOutDirName() << endl;
    }

    if (rank == TERRAIN_NODE_RANK) {
        if (verbose)
            cout << "[Terrain node] rank = " << rank << " running on: " << procname << endl;

        switch (terrain_type) {
            case ChVehicleCosimTerrainNode::Type::RIGID: {
#ifdef CHRONO_MULTICORE
                auto method = ChContactMethod::SMC;
                auto terrain = new ChVehicleCosimTerrainNodeRigid(method);
                terrain->SetVerbose(verbose);
                terrain->SetStepSize(step_size);
                terrain->SetOutDir(out_dir, suffix);
                terrain->EnableRuntimeVisualization(render, render_fps);
                if (verbose)
                    cout << "[Terrain node] output directory: " << terrain->GetOutDirName() << endl;

                terrain->SetPatchDimensions(terrain_length, terrain_width);

                terrain->SetFromSpecfile(terrain_specfile);

                node = terrain;
#endif
                break;
            }

            case ChVehicleCosimTerrainNode::Type::SCM: {
                auto terrain = new ChVehicleCosimTerrainNodeSCM(nthreads_terrain);
                terrain->SetVerbose(verbose);
                terrain->SetStepSize(step_size);
                terrain->SetOutDir(out_dir, suffix);
                terrain->EnableRuntimeVisualization(render, render_fps);
                if (verbose)
                    cout << "[Terrain node] output directory: " << terrain->GetOutDirName() << endl;

                terrain->SetPatchDimensions(terrain_length, terrain_width);

                terrain->SetFromSpecfile(terrain_specfile);

                if (use_checkpoint) {
                    terrain->SetInputFromCheckpoint("checkpoint_end.dat");
                }

                node = terrain;
                break;
            }

            case ChVehicleCosimTerrainNode::Type::GRANULAR_OMP: {
#ifdef CHRONO_MULTICORE
                auto method = ChContactMethod::SMC;
                auto terrain = new ChVehicleCosimTerrainNodeGranularOMP(method, nthreads_terrain);
                terrain->SetVerbose(verbose);
                terrain->SetStepSize(step_size);
                terrain->SetOutDir(out_dir, suffix);
                terrain->EnableRuntimeVisualization(render, render_fps);
                if (verbose)
                    cout << "[Terrain node] output directory: " << terrain->GetOutDirName() << endl;

                terrain->SetPatchDimensions(terrain_length, terrain_width);
                terrain->SetWallThickness(0.1);

                terrain->SetFromSpecfile(terrain_specfile);

                if (use_checkpoint) {
                    terrain->SetInputFromCheckpoint("checkpoint_settled.dat");
                } else {
                    terrain->SetSettlingTime(settling_time);
                    terrain->EnableSettlingOutput(settling_output, output_fps);
                    terrain->Settle();
                    terrain->WriteCheckpoint("checkpoint_settled.dat");
                }

                node = terrain;
#endif
                break;
            }

            case ChVehicleCosimTerrainNode::Type::GRANULAR_GPU: {
#ifdef CHRONO_GPU
                auto terrain = new ChVehicleCosimTerrainNodeGranularGPU();
                terrain->SetVerbose(verbose);
                terrain->SetStepSize(step_size);
                terrain->SetOutDir(out_dir, suffix);
                terrain->EnableRuntimeVisualization(render, render_fps);
                if (verbose)
                    cout << "[Terrain node] output directory: " << terrain->GetOutDirName() << endl;

                terrain->SetPatchDimensions(terrain_length, terrain_width);

                terrain->SetFromSpecfile(terrain_specfile);

                if (use_checkpoint) {
                    terrain->SetInputFromCheckpoint("checkpoint_settled.dat");
                } else {
                    terrain->SetSettlingTime(settling_time);
                    terrain->EnableSettlingOutput(settling_output, output_fps);
                    terrain->Settle();
                    terrain->WriteCheckpoint("checkpoint_settled.dat");
                }

                node = terrain;
#endif
                break;
            }

            case ChVehicleCosimTerrainNode::Type::GRANULAR_SPH: {
#ifdef CHRONO_FSI
                auto terrain = new ChVehicleCosimTerrainNodeGranularSPH();
                terrain->SetVerbose(verbose);
                std::string param_filename = GetChronoDataFile("fsi/input_json/demo_tire_rig.json");
                terrain->SetStepSize(step_size);
                terrain->SetOutDir(out_dir, suffix);
                terrain->EnableRuntimeVisualization(render, render_fps);
                if (verbose)
                    cout << "[Terrain node] output directory: " << terrain->GetOutDirName() << endl;

                terrain->SetPatchDimensions(terrain_length, terrain_width);

                terrain->SetFromSpecfile(terrain_specfile);

                node = terrain;
#endif
                break;
            }

        }  // switch terrain_type

    }  // if TERRAIN_NODE_RANK

    // Initialize systems.
    // Data exchange:
    //   terrain => rig (terrain height)
    //   rig => terrain (tire mesh topology and local vertex information)
    //   rig => terrain (tire contact material properties)
    node->Initialize();

    // Perform co-simulation.
    // At synchronization, there is bi-directional data exchange:
    //     rig => terrain (state information)
    //     terrain => rig (force information)
    int output_frame = 0;

    for (int is = 0; is < sim_steps; is++) {
        double time = is * step_size;

        if (verbose && rank == 0)
            cout << is << " ---------------------------- " << endl;
        MPI_Barrier(MPI_COMM_WORLD);

        node->Synchronize(is, time);
        node->Advance(step_size);
        if (verbose)
            cout << "Node" << rank << " sim time = " << node->GetSimTime() << "  [" << node->GetTotalSimTime() << "]"
                 << endl;

        if (sim_output && is % output_steps == 0) {
            node->OutputData(output_frame);
            output_frame++;
        }
    }

    node->WriteCheckpoint("checkpoint_end.dat");

    // Cleanup.
    delete node;
    MPI_Finalize();
    return 0;
}

// =============================================================================

bool GetProblemSpecs(int argc,
                     char** argv,
                     int rank,
                     std::string& terrain_specfile,
                     int& nthreads_rig,
                     int& nthreads_terrain,
                     double& step_size,
                     double& settling_time,
                     double& sim_time,
                     double& vel0,
                     double& slip,
                     double& sys_mass,
                     double& terrain_length,
                     double& terrain_width,
                     bool& use_checkpoint,
                     double& output_fps,
                     double& render_fps,
                     bool& sim_output,
                     bool& settling_output,
                     bool& render,
                     bool& verbose,
                     std::string& suffix) {
    ChCLI cli(argv[0], "Single-wheel test rig simulation.");

    cli.AddOption<std::string>("Demo", "terrain_specfile", "Terrain specification file [JSON format]");

    cli.AddOption<double>("Demo", "settling_time", "Duration of settling phase [s]", std::to_string(settling_time));
    cli.AddOption<double>("Demo", "sim_time", "Simulation length after settling phase [s]", std::to_string(sim_time));
    cli.AddOption<double>("Demo", "step_size", "Integration step size [s]", std::to_string(step_size));

    cli.AddOption<double>("Demo", "vel0", "Zero-splip tire linear velocity [m/s]", std::to_string(vel0));
    cli.AddOption<double>("Demo", "slip", "Longitudinal slip", std::to_string(slip));
    cli.AddOption<double>("Demo", "sys_mass", "Mass of wheel carrier [kg]", std::to_string(sys_mass));

    cli.AddOption<double>("Demo", "terrain_length", "Length of terrain patch [m]", std::to_string(terrain_length));
    cli.AddOption<double>("Demo", "terrain_width", "Width of terrain patch [m]", std::to_string(terrain_width));

    cli.AddOption<bool>("Demo", "use_checkpoint", "Initialize granular terrain from checkppoint file");

    cli.AddOption<bool>("Demo", "quiet", "Disable verbose messages");

    cli.AddOption<bool>("Demo", "no_render", "Disable OpenGL rendering");
    cli.AddOption<bool>("Demo", "no_output", "Disable generation of simulation result output files");
    cli.AddOption<bool>("Demo", "no_settling_output", "Disable generation of settling result output files");

    cli.AddOption<double>("Demo", "output_fps", "Output frequency [fps]", std::to_string(output_fps));
    cli.AddOption<double>("Demo", "render_fps", "Render frequency [fps]", std::to_string(render_fps));

    cli.AddOption<int>("Demo", "threads_rig", "Number of OpenMP threads for the rig node",
                       std::to_string(nthreads_rig));
    cli.AddOption<int>("Demo", "threads_terrain", "Number of OpenMP threads for the terrain node",
                       std::to_string(nthreads_terrain));

    cli.AddOption<std::string>("Demo", "suffix", "Suffix for output directory names", suffix);

    if (!cli.Parse(argc, argv)) {
        if (rank == 0)
            cli.Help();
        return false;
    }

    try {
        terrain_specfile = cli.Get("terrain_specfile").as<std::string>();
    } catch (std::domain_error& e) {
        if (rank == 0) {
            cout << "\nERROR: Missing terrain specification file!\n\n" << endl;
            cli.Help();
        }
        return false;
    }

    sim_time = cli.GetAsType<double>("sim_time");
    settling_time = cli.GetAsType<double>("settling_time");
    step_size = cli.GetAsType<double>("step_size");

    vel0 = cli.GetAsType<double>("vel0");
    slip = cli.GetAsType<double>("slip");
    sys_mass = cli.GetAsType<double>("sys_mass");

    terrain_length = cli.GetAsType<double>("terrain_length");
    terrain_width = cli.GetAsType<double>("terrain_width");

    verbose = !cli.GetAsType<bool>("quiet");
    render = !cli.GetAsType<bool>("no_render");
    sim_output = !cli.GetAsType<bool>("no_output");
    settling_output = !cli.GetAsType<bool>("no_settling_output");

    output_fps = cli.GetAsType<double>("output_fps");
    render_fps = cli.GetAsType<double>("render_fps");

    use_checkpoint = cli.GetAsType<bool>("use_checkpoint");

    nthreads_rig = cli.GetAsType<int>("threads_rig");
    nthreads_terrain = cli.GetAsType<int>("threads_terrain");

    suffix = cli.GetAsType<std::string>("suffix");

    return true;
}
