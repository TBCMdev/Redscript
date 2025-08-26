#pragma once

#include <memory>
#include "rbc_constant.hpp"

#include "../type_info.hpp"

struct rs_variable;
struct rs_var_access_path;

struct rs_var_access_path_item
{
    // rbc_constant: 0, 1, 5,
    // other: macrod in
    using _Value = std::variant<rbc_constant, std::shared_ptr<rs_variable>, std::shared_ptr<rs_var_access_path>>;
    
    _Value accessKey;
    bool   isArray; // false means its an object
};
struct rs_var_access_path
{
    std::shared_ptr<rs_variable>         fromVar = nullptr;
    std::vector<rs_var_access_path_item> segments = {};
    rs_type_info                         evaluatedType {}; // to get rid of warning

    std::string toPath() const;
    std::string toCompiledPath() const;
};
