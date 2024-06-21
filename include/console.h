#ifndef CONSOLE__H
#define CONSOLE__H

#include <map>
#include <span>
#include <string>
#include <string_view>

[[noreturn]] void uso_exe(std::string_view = {});
std::map<std::string, std::string> interpretar_args(std::span<const char*>);

#endif
// CONSOLE__H