#pragma once
#include <sstream>

#include "../token.hpp"
#include "../type_info.hpp"

#include "rs_compilation_info.hpp"
#include "rs_expression.hpp"

struct rs_object;

class rs_variable
{
public:
    token&       from;
    std::string  name;
    int32_t     scope;
    rs_type_info type_info, real_type_info;
    bool global = false; bool _const = false; bool ref = false;

    std::shared_ptr<rs_expression> value = nullptr;
    std::shared_ptr<rs_object> fromObject = nullptr;

    rs_compilation_info comp_info;

    rs_variable(token& _from, uint32_t _scope = 0, bool _global = false)
        : from(_from), name(_from.repr), scope(_scope), global(_global)
    {}
    rs_variable(token& _from, rs_type_info realInfo, int32_t _scope = 0, bool _global = false)
        : from(_from), name(_from.repr), scope(_scope), real_type_info(realInfo), global(_global)
    {}
    rs_variable(token& _from, rs_type_info explicitInfo, rs_type_info realInfo, int32_t _scope = 0, bool _global = false)
        : from(_from), name(_from.repr), scope(_scope), type_info(explicitInfo), real_type_info(realInfo), global(_global)
    {}

    inline std::string tostr()
    {
        std::stringstream stream;

        stream << '{' << from.str() << ", name=" << name << ", scope=" << scope << ", typeinf=" << type_info.tostr() << ", (g=" << global << ", c=" << _const << ")}";
        return stream.str();
    }
};