#pragma once
#include <memory>
#include <vector>
#include <string>

#include "rbc_types.hpp"

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

    std::string toPath();
};