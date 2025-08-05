#pragma once
#include <unordered_map>
#include <string>
#include <sstream>

#include "rs_variable.hpp"

class rs_macro
{
private:
public:
    std::shared_ptr<rs_variable> var;

    explicit rs_macro(const std::shared_ptr<rs_variable>& _var) : var(_var) {}
};

struct rs_macro_container
{
    std::unordered_map<std::shared_ptr<rs_variable>, rs_macro> macros;

    inline void add(const std::shared_ptr<rs_variable>& var)
    {
        macros.insert({var, rs_macro(var)});
    }

    std::string constructMacroPayload() const;
};
