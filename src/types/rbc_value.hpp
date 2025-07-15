#pragma once

#include <variant>

#include "rbc_constant.hpp"
#include "rs_var_access_path.hpp"

struct rbc_register;
struct rs_variable;
struct rs_object;
struct rs_list;

/*
All types that can be a parameter of a byte-code assembly instruction.
- constants, which dont need to be referenced and can be copied.
- registers, operable & non operable, used for primarily arithmetic
- objects, either JSON-inline objects or class type object instances. TODO change from rs_object to rs_object_instance
- lists, lists
- void*, weird to have but are used to pass critical info to an instruction.
    example:
        a function call: sometimes a function call can exist somewhere where it is impossible to find,
        like x(){y(){z(){}w(){}}}
        if I call w() inside x, how does the instruction find w efficiently? it could be a global function,
        a function from a module, etc.
        so we use void* to pass the function* through to the instruction to save time.
- variable usage paths: x[0][2][3].b.a[1].c
*/
#define RBC_VALUE_T std::variant<rbc_constant,       \
                      std::shared_ptr<rbc_register>, \
                      std::shared_ptr<rs_variable>,  \
                      std::shared_ptr<rs_object>,    \
                      std::shared_ptr<rs_list>,      \
                      std::shared_ptr<void>,         \
                      rs_var_access_path>

using rbc_value = RBC_VALUE_T;