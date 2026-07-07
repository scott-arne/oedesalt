#include "oedesalt/oedesalt.h"

#include <oechem.h>

namespace OEDESALT {

double calculate_molecular_weight(const OEChem::OEMolBase& mol) {
    return OEChem::OECalculateMolecularWeight(mol);
}

} // namespace OEDESALT
