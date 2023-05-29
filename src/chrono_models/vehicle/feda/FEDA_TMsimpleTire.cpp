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
// Authors: Rainer Gericke
// =============================================================================
//
// U401 TMeasy tire subsystem
//
// =============================================================================

#include <algorithm>
#include <cmath>

#include "chrono_models/vehicle/feda/FEDA_TMsimpleTire.h"
#include "chrono_vehicle/ChVehicleModelData.h"

namespace chrono {
namespace vehicle {
namespace feda {

// -----------------------------------------------------------------------------
// Static variables
// -----------------------------------------------------------------------------

const std::string FEDA_TMsimpleTire::m_meshFile_left = "feda/meshes/FEDA_tire_fine.obj";
const std::string FEDA_TMsimpleTire::m_meshFile_right = "feda/meshes/FEDA_tire_fine.obj";

const double FEDA_TMsimpleTire::m_mass = 56.1;
const ChVector<> FEDA_TMsimpleTire::m_inertia(10.5, 16.8, 10.5);

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
FEDA_TMsimpleTire::FEDA_TMsimpleTire(const std::string& name) : ChTMsimpleTire(name) {
    SetTMsimpleParams();
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void FEDA_TMsimpleTire::SetTMsimpleParams() {
    m_unloaded_radius = 0.4987;  //$Free tyre radius
    m_width = 0.3350;            //$Nominal section width of the tyre
    m_rim_radius = 0.2858;       //$Nominal rim radius
    m_rolling_resistance = 0.015;

    m_TMsimpleCoeff.dz = 6188.0;
    m_TMsimpleCoeff.pn = 21674.0 / 2.0;
    m_TMsimpleCoeff.pn_max = 3.5 * m_TMsimpleCoeff.pn;
    
    m_TMsimpleCoeff.dfx0_pn = 131379.8988;
    m_TMsimpleCoeff.dfx0_p2n = 289802.2285;
    m_TMsimpleCoeff.fxm_pn = 10148.8286;
    m_TMsimpleCoeff.fxm_p2n = 19437.8446;
    m_TMsimpleCoeff.fxs_pn = 7948.8727;
    m_TMsimpleCoeff.fxs_p2n = 15723.0773;
    m_TMsimpleCoeff.dfy0_pn = 125823.3139;
    m_TMsimpleCoeff.dfy0_p2n = 227719.1423;
    m_TMsimpleCoeff.fym_pn = 8352.7333;
    m_TMsimpleCoeff.fym_p2n = 15635.4453;
    m_TMsimpleCoeff.fys_pn = 8174.1916;
    m_TMsimpleCoeff.fys_p2n = 15344.1442;

    SetVerticalStiffness(618787.0);
    SetHorizontalCoefficients();

}

void FEDA_TMsimpleTire::GenerateCharacteristicPlots(const std::string& dirname) {
    // Write a plot file (gnuplot) to check the tire characteristics.
    // Inside gnuplot use the command load 'filename'
    std::string filename = dirname + "365_65_R20_60psi" + GetName() + ".gpl";
    WritePlots(filename, "365_65_R20_60psi");
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void FEDA_TMsimpleTire::AddVisualizationAssets(VisualizationType vis) {
    if (vis == VisualizationType::MESH) {
        m_trimesh_shape = AddVisualizationMesh(m_meshFile_left,    // left side
                                               m_meshFile_right);  // right side
    } else {
        ChTMsimpleTire::AddVisualizationAssets(vis);
    }
}

void FEDA_TMsimpleTire::RemoveVisualizationAssets() {
    ChPart::RemoveVisualizationAsset(m_wheel->GetSpindle(), m_trimesh_shape);
    ChTMsimpleTire::RemoveVisualizationAssets();
}

}  // namespace feda
}  // end namespace vehicle
}  // end namespace chrono
