#pragma once
#include <vector>
#include <memory>

#include "rbc_value.hpp"

#include "../type_info.hpp"

struct rs_list
{
    rs_type_info elementType;
    std::vector<std::shared_ptr<rbc_value>> values;
};