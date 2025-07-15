#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <sstream>

#include "rs_variable.hpp"

enum class rs_object_member_decorator
{
    OPTIONAL,
    REQUIRED,
    SEPERATE,
};
struct rs_object
{
    const static uint32_t TYPE_CARET_START = 10; // custom types start at 10 and onwards for type id.
    using _MemberT = std::pair<rs_variable, rs_object_member_decorator>;
    // x.name -> 
    std::string name; // empty for inline objects
    int32_t scope;
    // negative for inline created objects
    int32_t typeID = -1;
    std::unordered_map<std::string, _MemberT> members = {};

    inline std::string tostr()
    {
        std::stringstream stream;

        stream << "(obj)" << (name.empty() ? "{inline=1" : "{name=" + name) << ", members={";
        int i = 0;
        for(auto& member : members)
        {
            if (i != 0)
                stream << ',';
            stream << member.second.first.name;
            i++;
        }
        stream << "}}";
        return stream.str();
    }
};
