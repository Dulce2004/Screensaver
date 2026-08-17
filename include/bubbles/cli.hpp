#pragma once

#include "bubbles/types.hpp"

#include <iosfwd>
#include <string>

namespace bubbles {

void printUsage(std::ostream& output);
bool parseCommandLine(
    int argc,
    char** argv,
    ProgramOptions& options,
    std::string& errorMessage);

}  // namespace bubbles

