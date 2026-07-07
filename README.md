# OEDesalt

A small, fast salt and solvent remover for [OpenEye Toolkits](https://www.eyesopen.com/)
molecules, available as both a C++ library and a Python package.

Many structures arrive as a drug plus a counterion, salt, or crystallization
solvent — `aspirin.HCl`, `amine.oxalic acid`, `drug.water`. OEDesalt strips those
extra components and leaves the compound of interest, using a curated set of
salt/solvent patterns that are compiled into the library (no data files to ship
or configure).

```python
from openeye import oechem
from oedesalt import Desalter

mol = oechem.OEGraphMol()
oechem.OESmilesToMol(mol, "CC(=O)Oc1ccccc1C(=O)O.Cl")   # aspirin . HCl

result = Desalter().desalt(mol)
print(oechem.OEMolToSmiles(result.mol))   # CC(=O)Oc1ccccc1C(=O)O  (aspirin)
print(result.stripped_names)              # ['Halides']
```

## How it works

- **Whole-fragment matching.** A disconnected component is removed only if a
  pattern matches the *entire* fragment. A covalently bound chlorine on your
  molecule is never mistaken for a chloride counterion.
- **Charge-agnostic.** `[Na]` matches `Na+`, `C(=O)O` matches a carboxylate, and
  so on — you don't need charged input.
- **Solvents are opt-in.** By default only salts/counterions are removed. Water
  and other crystallization solvents are kept unless you ask for them
  (`strip_solvents=True`).
- **Single-component inputs are left alone.** A lone molecule that happens to
  look like a salt-former (pyridine, a fatty acid) is the compound of interest,
  not a salt, so it is returned untouched — unless you opt into `aggressive` mode.
- **Pure removal.** Components are deleted; nothing is neutralized or re-protonated,
  and the molecule title is preserved.

## Requirements

- **OpenEye Toolkits** with a valid license (`OE_LICENSE`). The Python package
  depends on `openeye-toolkits`; the C++ library needs the OpenEye C++ SDK.
- **Python** ≥ 3.10 (for the Python package).
- **C++17** and **CMake** ≥ 3.16 (for the C++ library).

## Python

### Install

```bash
pip install oedesalt
```

This pulls in `openeye-toolkits` automatically. Make sure your OpenEye license is
available at runtime (for example, `export OE_LICENSE=/path/to/oe_license.txt`).

### Usage

Build a `Desalter` once and reuse it — it is immutable and cheap to construct.
Pass any `openeye.oechem` molecule to `desalt()`:

```python
from openeye import oechem
from oedesalt import Desalter

desalter = Desalter()

mol = oechem.OEGraphMol()
oechem.OESmilesToMol(mol, "c1ccncc1C(=O)NC.Cl.[Na]")

result = desalter.desalt(mol)
result.mol             # the surviving molecule (an OEGraphMol)
result.stripped_names  # e.g. ['Halides', 'Alkali metals'] — patterns that removed a component
```

`desalt()` returns a `DesaltResult` with two attributes:

- `mol` — the surviving molecule. It is empty if *every* component matched a
  pattern (for example, desalting plain `[Na].Cl`).
- `stripped_names` — the display names of the patterns that removed a component.
  Empty when nothing was stripped.

### Configuration

Everything is set on the constructor:

```python
Desalter()                             # bundled salts; solvents left in place (default)
Desalter(strip_solvents=True)          # also strip bundled solvents (water, etc.)
Desalter(aggressive=True)              # strip salt-formers from single-component inputs too
Desalter(salt_file="my_salts.smarts")  # use your own salt patterns instead of the bundled set
Desalter(salt_file="my_salts.smarts", solvent_file="my_solvents.smarts")  # custom both
```

Notes:

- Passing a `solvent_file` implies `strip_solvents=True`.
- `strip_solvents=True` with no `solvent_file` uses the bundled solvent set.
- `aggressive=True` desalts single-component inputs, which can empty the molecule
  entirely if the whole thing matches a pattern — use it deliberately.

### Errors

Loading a bad pattern file raises a typed exception, all subclasses of
`OEDesaltError`:

```python
from oedesalt import Desalter, PatternFileError, InvalidPatternError, OEDesaltError

try:
    Desalter(salt_file="does_not_exist.smarts")
except PatternFileError:      # file could not be opened
    ...
except InvalidPatternError:   # a SMARTS line failed to parse
    ...
```

## C++

### Consuming the library

OEDesalt installs a static library, its headers, and a CMake targets file. The
`OEDESALT::oedesalt` target expects OpenEye to be available in your build, which
is provided by the [`cmake-openeye`](https://github.com/scott-arne/cmake-openeye)
helper.

```cmake
cmake_minimum_required(VERSION 3.16)
project(my_app LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# Provide find_package(OpenEye) via the cmake-openeye helper.
include(FetchContent)
FetchContent_Declare(cmake_openeye
    GIT_REPOSITORY https://github.com/scott-arne/cmake-openeye.git
    GIT_TAG v1.2.1 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(cmake_openeye)
list(APPEND CMAKE_MODULE_PATH "${cmake_openeye_SOURCE_DIR}")
find_package(OpenEye REQUIRED)

# Import the installed oedesalt target.
include(<install-prefix>/lib/cmake/oedesalt/OEDESALTTargets.cmake)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE OEDESALT::oedesalt OpenEye::OEChem)
```

Configure with `-DOPENEYE_ROOT=/path/to/openeye/sdk` so `find_package(OpenEye)`
can locate the SDK.

### Usage

```cpp
#include "oedesalt/Desalter.h"
#include <oechem.h>

OEChem::OEGraphMol mol;
OEChem::OESmilesToMol(mol, "CC(=O)Oc1ccccc1C(=O)O.Cl");   // aspirin . HCl

// Build from the patterns compiled into the library — no data files needed.
const OEDESALT::Desalter desalter = OEDESALT::Desalter::WithBundledPatterns();
const OEDESALT::DesaltResult result = desalter.Desalt(mol);

result.mol;              // surviving molecule (empty if everything matched)
result.stripped_names;   // std::vector<std::string> of removed pattern names
```

`WithBundledPatterns(strip_solvents = false, aggressive = false)` mirrors the
Python constructor. To load your own patterns from files, use
`OEDESALT::Desalter::FromFiles(salt_path, solvent_path = "", aggressive = false)`.
Pattern-file problems throw `OEDESALT::PatternFileError` or
`OEDESALT::InvalidPatternError` (both derive from `OEDESALT::OEDesaltError`).

## Custom pattern files

Both APIs accept your own pattern files in a simple RDKit-style format: one
pattern per line, a SMARTS token followed by a display name. Lines starting with
`//` or `#`, and blank lines, are ignored.

```text
// my_salts.smarts
[F,Cl,Br,I]        Halides
[Li,Na,K,Rb,Cs]    Alkali metals
C(=O)O             Carboxylate counterion
```

Patterns use whole-fragment, charge-agnostic matching, so **do not** encode
formal charges — write `[Na]`, not `[Na+]`.

## License

MIT — see [LICENSE](LICENSE).
