#pragma once

#include <string>

/** Code that the host tools share. */
namespace Tools::Common {

/** Kind of failure a tool reports. */
enum class ErrorCode {
    InvalidInput, /*!< An input file does not have the expected format. */
    Io,           /*!< Reading or writing a file failed. */
};

/** Failure returned through `std::expected`. */
struct Error {
    ErrorCode code;      /*!< Kind of failure. */
    std::string message; /*!< Sentence describing the failure, for the log. */
};

} // namespace Tools::Common
