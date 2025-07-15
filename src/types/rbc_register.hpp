#pragma once
#include <cstdint>
#include <string>

class rbc_register
{
public:
    uint32_t id;
    bool operable;
    bool vacant = true;

    rbc_register(uint32_t _id, bool _operable, bool _vacant)
        : id(_id), operable(_operable), vacant(_vacant)
    {
    }
    inline void free()
    {
        vacant = true;
    }
    inline std::string tostr()
    {
        return "(reg){(id=" + std::to_string(id) + ") op=" + std::to_string(operable) + ", vacant=" + std::to_string(vacant) + '}';
    }
};