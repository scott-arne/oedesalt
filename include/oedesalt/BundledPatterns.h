#ifndef OEDESALT_BUNDLED_PATTERNS_H
#define OEDESALT_BUNDLED_PATTERNS_H

namespace OEDESALT {
namespace detail {

/// \brief Embedded default salt SMARTS (generated from salts.smarts at build time).
extern const char* const kBundledSalts;

/// \brief Embedded default solvent SMARTS (generated from solvents.smarts at build time).
extern const char* const kBundledSolvents;

}  // namespace detail
}  // namespace OEDESALT

#endif  // OEDESALT_BUNDLED_PATTERNS_H
