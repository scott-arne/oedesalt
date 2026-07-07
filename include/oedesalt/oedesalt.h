#ifndef OEDESALT_H
#define OEDESALT_H

// Version information
#define OEDESALT_VERSION_MAJOR 0
#define OEDESALT_VERSION_MINOR 1
#define OEDESALT_VERSION_PATCH 0

#include <oechem.h>

namespace OEDESALT {

/// \brief Calculate the molecular weight of a molecule.
///
/// Passes the molecule natively from Python to C++ via SWIG typemaps,
/// then delegates to OEChem's OECalculateMolecularWeight.
///
/// \param mol Reference to an OEMolBase object.
/// \returns Molecular weight in Daltons.
double calculate_molecular_weight(const OEChem::OEMolBase& mol);

} // namespace OEDESALT

#endif // OEDESALT_H
