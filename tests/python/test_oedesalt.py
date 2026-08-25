"""Tests for the oedesalt Python facade."""

from openeye import oechem


def _smiles(mol):
    return oechem.OEMolToSmiles(mol)


def test_version_available():
    import oedesalt

    assert hasattr(oedesalt, "__version__")
    assert hasattr(oedesalt, "__version_info__")
    assert oedesalt.__version__ == "0.1.2"
    assert oedesalt.__version_info__ == (0, 1, 2)


def test_bare_desalter_strips_halide(aspirin_hcl_mol):
    import oedesalt

    result = oedesalt.Desalter().desalt(aspirin_hcl_mol)
    assert "Halides" in result.stripped_names
    assert "." not in _smiles(result.mol)


def test_desalt_returns_result_object(aspirin_hcl_mol):
    import oedesalt

    result = oedesalt.Desalter().desalt(aspirin_hcl_mol)
    assert isinstance(result, oedesalt.DesaltResult)
    assert isinstance(result.mol, oechem.OEMolBase)
    assert isinstance(result.stripped_names, list)


def test_solvents_off_by_default(drug_water_mol):
    import oedesalt

    # Bare Desalter() must NOT strip the water component, and must strip nothing.
    result = oedesalt.Desalter().desalt(drug_water_mol)
    assert "." in _smiles(result.mol)
    assert result.stripped_names == []


def test_strip_solvents_removes_water(drug_water_mol):
    import oedesalt

    result = oedesalt.Desalter(strip_solvents=True).desalt(drug_water_mol)
    assert "." not in _smiles(result.mol)


def test_single_component_salt_former_survives_by_default(pyridine_mol):
    import oedesalt

    result = oedesalt.Desalter().desalt(pyridine_mol)
    assert result.stripped_names == []
    assert _smiles(result.mol) == _smiles(pyridine_mol)


def test_aggressive_strips_single_component_salt_former(pyridine_mol):
    import oedesalt

    result = oedesalt.Desalter(aggressive=True).desalt(pyridine_mol)
    # Pyridine matches a bundled pattern; aggressive empties the molecule.
    assert result.mol.NumAtoms() == 0
    assert result.stripped_names


def test_pattern_count_and_aggressive_properties():
    import oedesalt

    salts_only = oedesalt.Desalter()
    with_solvents = oedesalt.Desalter(strip_solvents=True)
    assert salts_only.pattern_count > 0
    assert with_solvents.pattern_count > salts_only.pattern_count
    assert oedesalt.Desalter(aggressive=True).aggressive is True
    assert salts_only.aggressive is False


def test_immutable_facade():
    import oedesalt

    import pytest

    d = oedesalt.Desalter()
    with pytest.raises(AttributeError):
        d.aggressive = True


def test_custom_salt_file_override(tmp_path, mol_factory):
    import oedesalt

    salt_file = tmp_path / "mini.smarts"
    salt_file.write_text("[F,Cl,Br,I]   Halides\n", encoding="utf-8")
    result = oedesalt.Desalter(salt_file=salt_file).desalt(
        mol_factory("CC(=O)Oc1ccccc1C(=O)O.Cl")
    )
    assert "Halides" in result.stripped_names


def test_custom_salt_file_with_bundled_solvents(tmp_path, drug_water_mol):
    import oedesalt

    # Custom salts + strip_solvents=True (no solvent_file) must still pull in the
    # bundled solvents and strip the water.
    salt_file = tmp_path / "mini.smarts"
    salt_file.write_text("[F,Cl,Br,I]   Halides\n", encoding="utf-8")
    result = oedesalt.Desalter(salt_file=salt_file, strip_solvents=True).desalt(drug_water_mol)
    assert "." not in _smiles(result.mol)


def test_missing_file_raises_pattern_file_error():
    import oedesalt

    import pytest

    with pytest.raises(oedesalt.PatternFileError):
        oedesalt.Desalter(salt_file="/no/such/file.smarts")


def test_malformed_smarts_raises_invalid_pattern_error(tmp_path):
    import oedesalt

    import pytest

    bad = tmp_path / "bad.smarts"
    bad.write_text("[Cl   Halide\n", encoding="utf-8")
    with pytest.raises(oedesalt.InvalidPatternError):
        oedesalt.Desalter(salt_file=bad)


def test_errors_share_base_class(tmp_path):
    import oedesalt

    import pytest

    with pytest.raises(oedesalt.OEDesaltError):
        oedesalt.Desalter(salt_file="/no/such/file.smarts")


def test_macos_skips_openeye_preload_without_version_drift():
    """On macOS with no version drift, importing oedesalt must not eagerly
    preload the OpenEye libraries.

    The extension resolves its OpenEye dependencies through its own absolute
    LC_RPATH, so an eager RTLD_GLOBAL preload at import time is unnecessary and
    perturbs process-global load order -- which can tip order-sensitive
    conflicts with other OpenMP-linked extensions (scikit-learn, PyTorch) into a
    hard abort(). This guards that _preload_shared_libs issues no library loads
    in that case. See BUG_REPORT_openmp_abort.md.
    """
    import sys

    import pytest

    if sys.platform != "darwin":
        pytest.skip("preload-skip behavior is macOS-specific")

    import ctypes

    import oedesalt

    # No version drift in a normal environment: the drift signal is unset.
    assert oedesalt._OPENEYE_COMPAT_EXTENSION_DIR is None

    calls = []
    original_cdll = ctypes.CDLL
    try:
        ctypes.CDLL = lambda *a, **k: calls.append((a, k))  # type: ignore[assignment]
        oedesalt._preload_shared_libs()
    finally:
        ctypes.CDLL = original_cdll

    assert calls == [], f"expected no preload CDLL calls on macOS, got {calls}"
