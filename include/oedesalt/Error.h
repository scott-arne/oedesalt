#ifndef OEDESALT_ERROR_H
#define OEDESALT_ERROR_H

#include <stdexcept>
#include <string>

namespace OEDESALT {

/// \brief Base class for all oedesalt errors.
class OEDesaltError : public std::runtime_error {
public:
    explicit OEDesaltError(const std::string& message)
        : std::runtime_error(message) {}
};

/// \brief Raised when a salt/solvent pattern file cannot be opened.
class PatternFileError : public OEDesaltError {
public:
    explicit PatternFileError(const std::string& message)
        : OEDesaltError(message) {}
};

/// \brief Raised when a SMARTS line fails to parse. The message names the
/// source (file path or embedded label) and the offending line number.
class InvalidPatternError : public OEDesaltError {
public:
    explicit InvalidPatternError(const std::string& message)
        : OEDesaltError(message) {}
};

}  // namespace OEDESALT

#endif  // OEDESALT_ERROR_H
