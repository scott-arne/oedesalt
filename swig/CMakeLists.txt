include(OpenEyeSWIG)

openeye_add_swig_module(
    NAME oedesalt
    SWIG_FILE oedesalt.i
    LINK_LIBS oedesalt
    PYTHON_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/python/oedesalt
    STABLE_ABI ${OEDESALT_USE_STABLE_ABI}
    SWIG_FLAGS "-doxygen"
    EXPECTED_LIB_VARS
        OECHEM_LIBRARY OESYSTEM_LIBRARY OEPLATFORM_LIBRARY
        OEMATH_LIBRARY OEZSTD_LIBRARY
    INIT_PY ${CMAKE_SOURCE_DIR}/python/oedesalt/__init__.py
)
