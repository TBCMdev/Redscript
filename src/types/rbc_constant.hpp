#pragma once
#include <string>

#include "../token.hpp"

class rbc_constant
{
public:
    inline void quoteIfStr()
    {
        if (val_type == token_type::STRING_LITERAL)
            val = '"' + val + '"';
    }
    inline std::string quoted()
    {
        return '"' + val + '"';
    }
    const token_type val_type;
    std::string val;
    std::shared_ptr<raw_trace_info> trace = nullptr;
    rbc_constant(token_type _val_type, std::string _val)
        : val_type(_val_type), val(_val)
    {
    }
    rbc_constant(token_type _val_type, std::string _val, std::shared_ptr<raw_trace_info> _trace)
        : val_type(_val_type), val(_val), trace(_trace)
    {
    }
    inline std::string tostr()
    {
        return "(const){T=" + std::to_string(static_cast<uint32_t>(val_type)) + ", v=" + val + '}';
    }
};