#include "oedesalt/Desalter.h"
#include <oechem.h>

#include <iostream>

int main() {
    OEChem::OEGraphMol mol;
    if (!OEChem::OESmilesToMol(mol, "CC(=O)Oc1ccccc1C(=O)O.Cl")) {
        std::cerr << "could not parse the input SMILES\n";
        return 1;
    }

    // The bundled patterns are compiled into the library, so this also checks
    // that the installed archive carries them rather than relying on data files.
    const OEDESALT::Desalter desalter = OEDESALT::Desalter::WithBundledPatterns();
    const OEDESALT::DesaltResult result = desalter.Desalt(mol);
    if (result.stripped_names.empty()) {
        std::cerr << "expected the HCl counterion to be stripped\n";
        return 1;
    }

    std::cout << "stripped:";
    for (const auto& name : result.stripped_names) {
        std::cout << ' ' << name;
    }
    std::cout << '\n';
    return 0;
}
