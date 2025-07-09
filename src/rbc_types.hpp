#pragma once
#include <string>
#include <cstdint>
#include "token.hpp"

#define RBC_CONST_INT_ID 0
#define RBC_CONST_STR_ID 1
#define RBC_CONST_FLOAT_ID 2
#define RBC_CONST_LIST_ID 3

struct project_fragment
{
    std::string fileName;
    std::string fileContent;

    token_list tokens;
};
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

typedef std::deque<std::shared_ptr<project_fragment>> fragment_ptr_deque;
struct                                                rbc_function; // for impls, defined in rbc.cpp. Maybe change?
struct                                                rbc_program;
typedef std::pair<std::shared_ptr<rs_variable>, bool> rbc_func_var_t;

typedef RBC_VALUE_T                                   rbc_value; // also defined in types.hpp. Remove?
