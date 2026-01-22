#pragma once
#include <string_view>
#include "tsexpr/program.hpp"

namespace tsexpr {

// Compile a single statement: IDENT '=' EXPR
Program compile(std::string_view input);

} // namespace tsexpr
