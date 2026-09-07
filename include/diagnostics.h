#pragma once
#include <string>
namespace Diagnostics {
void init(const std::string& directory);
bool enabled();
void log(const char* format, ...);
void error(const char* message);
}
