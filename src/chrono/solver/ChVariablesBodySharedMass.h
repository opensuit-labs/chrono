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
// Authors: Alessandro Tasora, Radu Serban
// =============================================================================

#ifndef CHVARIABLESBODYSHAREDMASS_H
#define CHVARIABLESBODYSHAREDMASS_H

#include "chrono/solver/ChVariablesBody.h"

namespace chrono {

/// Used by ChVariablesBodySharedMass objects to reference a single mass property.

class ChApi ChSharedMassBody {
  public:
    ChMatrix33<double> inertia;      ///< 3x3 inertia matrix
    double mass;                     ///< mass value
    ChMatrix33<double> inv_inertia;  ///< inverse of inertia matrix
    double inv_mass;                 ///< inverse of mass value

  public:
    ChSharedMassBody();

    /// Set the inertia matrix
    void SetBodyInertia(const ChMatrix33<>& minertia);

    /// Set the mass associated with translation of body
    void SetBodyMass(const double mmass);

    /// Access the 3x3 inertia matrix
    ChMatrix33<>& GetBodyInertia() { return inertia; }
    const ChMatrix33<>& GetBodyInertia() const { return inertia; }

    /// Access the 3x3 inertia matrix inverted
    ChMatrix33<>& GetBodyInvInertia() { return inv_inertia; }
    const ChMatrix33<>& GetBodyInvInertia() const { return inv_inertia; }

    /// Get the mass associated with translation of body
    double GetBodyMass() const { return mass; }

    /// Method to allow serialization of transient data to archives.
    void ArchiveOut(ChArchiveOut& archive_out);

    /// Method to allow de-serialization of transient data from archives.
    void ArchiveIn(ChArchiveIn& archive_in);
};

/// Specialized class for representing a 6-DOF item for a  system, that is a 3D rigid body, with mass matrix and
/// associate variables (a 6 element vector, ex.speed)  Differently from the 'naive' implementation ChVariablesGeneric,
/// here a full 6x6 mass matrix is not built, since only the 3x3  inertia matrix and the mass value are enough.  This is
/// very similar to ChVariablesBodyOwnMass, but the  mass and inertia values are shared, that can be useful for problems
/// with thousands of equally-shaped objects.

class ChApi ChVariablesBodySharedMass : public ChVariablesBody {
  private:
    ChSharedMassBody* sharedmass;  ///< shared inertia properties

  public:
    ChVariablesBodySharedMass();

    virtual ~ChVariablesBodySharedMass() {}

    /// Assignment operator: copy from other object
    ChVariablesBodySharedMass& operator=(const ChVariablesBodySharedMass& other);

    /// Get the pointer to shared mass
    ChSharedMassBody* GetSharedMass() { return sharedmass; }

    /// Set pointer to shared mass
    void SetSharedMass(ChSharedMassBody* ms) { sharedmass = ms; }

    /// Get the mass associated with translation of body
    virtual double GetBodyMass() const override { return sharedmass->GetBodyMass(); }

    /// Access the 3x3 inertia matrix
    virtual ChMatrix33<>& GetBodyInertia() override { return sharedmass->GetBodyInertia(); }
    virtual const ChMatrix33<>& GetBodyInertia() const override { return sharedmass->GetBodyInertia(); }

    /// Access the 3x3 inertia matrix inverted
    virtual ChMatrix33<>& GetBodyInvInertia() override { return sharedmass->GetBodyInvInertia(); }
    virtual const ChMatrix33<>& GetBodyInvInertia() const override { return sharedmass->GetBodyInvInertia(); }

    /// Computes the product of the inverse mass matrix by a vector, and set in result: result = [invMb]*vect
    virtual void Compute_invMb_v(ChVectorRef result, ChVectorConstRef vect) const override;

    /// Computes the product of the inverse mass matrix by a vector, and increment result: result += [invMb]*vect
    virtual void Compute_inc_invMb_v(ChVectorRef result, ChVectorConstRef vect) const override;

    /// Computes the product of the mass matrix by a vector, and set in result: result = [Mb]*vect
    virtual void Compute_inc_Mb_v(ChVectorRef result, ChVectorConstRef vect) const override;

    /// Computes the product of the corresponding block in the system matrix (ie. the mass matrix) by 'vect', scale by
    /// ca, and add to 'result'.
    /// NOTE: the 'vect' and 'result' vectors must already have the size of the total variables&constraints in the
    /// system; the procedure will use the ChVariable offsets (that must be already updated) to know the indexes in
    /// result and vect.
    virtual void MultiplyAndAdd(ChVectorRef result, ChVectorConstRef vect, const double ca) const override;

    /// Add the diagonal of the mass matrix scaled by ca, to 'result'.
    /// NOTE: the 'result' vector must already have the size of system unknowns, ie the size of the total variables &
    /// constraints in the system; the procedure will use the ChVariable offset (that must be already updated) as index.
    virtual void DiagonalAdd(ChVectorRef result, const double ca) const override;

    /// Write the mass submatrix for these variables into the specified global matrix at the offsets of each variable.
    /// The masses must be scaled by the given factor 'ca').
    virtual void PasteMassInto(ChSparseMatrix& storage,
                               unsigned int row_offset,
                               unsigned int col_offset,
                               const double ca) const override;

    /// Method to allow serialization of transient data to archives.
    virtual void ArchiveOut(ChArchiveOut& archive_out) override;

    /// Method to allow de-serialization of transient data from archives.
    virtual void ArchiveIn(ChArchiveIn& archive_in) override;
};

}  // end namespace chrono

#endif
