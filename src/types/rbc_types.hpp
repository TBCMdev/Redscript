#pragma once
#include <string>
#include <cstdint>

#include "../token.hpp"
#include "rs_variable.hpp"

#define RBC_CONST_INT_ID 0
#define RBC_CONST_STR_ID 1
#define RBC_CONST_FLOAT_ID 2
#define RBC_CONST_LIST_ID 3


enum class rbc_instruction
{
    CREATE,
    CALL,
    SAVE,
    MATH,
    DEL,
    EQ,
    NEQ,
    GT,
    LT,

    IF,
    NIF,
    ENDIF,
    ELSE,
    ELIF,
    NELIF,

    RET,
    SAVERET,
    PUSH,
    POP,
    INC, // inc scope
    DEC  // dec scope
};
enum class rbc_scope_type
{
    IF,
    ELIF,
    ELSE,
    FUNCTION,
    MODULE,
    NONE
};
enum class rbc_value_type
{
    CONSTANT,
    NBT,
    OREG,
    N_OREG,
    OBJECT
};
enum class rbc_function_decorator
{
    EXTERN, // inbuilt functions
    SINGLE, // functions with 1 single call, redundant to compile.
    CPP,
    NOCOMPILE,
    NORETURN,
    WRAPPER,
    UNKNOWN
};



struct                                                rbc_function; // for impls, defined in rbc.cpp. Maybe change?
struct                                                rbc_program;
typedef std::pair<std::shared_ptr<rs_variable>, bool> rbc_func_var_t;
