#pragma once
#include "../util.hpp"

struct rs_stack_frame
{
    short id = 0;
};

struct rs_compilation_info
{
    int varIndex = 0;
    shared_wrapper<rs_stack_frame> belongingStackFrame = nullptr;
    bool isParameter = false;




    inline short get_stackframe_id() const
    {
        return belongingStackFrame ? belongingStackFrame->id : 0;
    }
};