#pragma once

#include <memory>
#include "rbc_constant.hpp"

#include "../type_info.hpp"

struct rs_variable;

struct rs_var_access_path_item
{
    using _Value = rbc_constant;
    
    _Value accessKey;
    bool   isArray; // false means its an object
};
struct rs_var_access_path
{
    std::shared_ptr<rs_variable>         fromVar = nullptr;
    std::vector<rs_var_access_path_item> segments = {};
    rs_type_info                         evaluatedType {}; // to get rid of warning

    std::string toPath();
    std::string toCompiledPath();
};
