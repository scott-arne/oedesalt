"""Shared fixtures and configuration for oedesalt Python tests."""

import pytest

pytest.importorskip("openeye.oechem", reason="OpenEye Toolkits not installed")


def _mol(smiles, title=""):
    from openeye import oechem

    mol = oechem.OEGraphMol()
    oechem.OESmilesToMol(mol, smiles)
    if title:
        mol.SetTitle(title)
    return mol


@pytest.fixture
def aspirin_hcl_mol():
    """Aspirin with an HCl counterion (two components)."""
    return _mol("CC(=O)Oc1ccccc1C(=O)O.Cl", "aspirin_hcl")


@pytest.fixture
def drug_water_mol():
    """A drug fragment with a water solvate (two components)."""
    return _mol("c1ccncc1C(=O)NC.O", "drug_water")


@pytest.fixture
def pyridine_mol():
    """Pyridine — a single-component salt-former."""
    return _mol("c1ccncc1", "pyridine")


@pytest.fixture
def mol_factory():
    """Return a helper that builds an OEGraphMol from SMILES (+ optional title)."""
    return _mol
