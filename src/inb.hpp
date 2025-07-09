#pragma once
#include <unordered_map>
#include <vector>
#include <variant>
#include <memory>

#include "globals.hpp"

// forward decls for rbc_value
struct rbc_constant;
struct rbc_register;
struct rs_variable;
struct rs_object;
struct rs_list;
struct rs_var_access_path;
typedef RBC_VALUE_T rbc_value;



namespace conversion
{
    class CommandFactory;
};
struct rbc_program;
#define INB_IMPL_PARAMETERS [[maybe_unused]] rbc_program& program,                \
                            [[maybe_unused]] conversion::CommandFactory& factory, \
                            [[maybe_unused]] std::vector<rbc_value>& parameters,  \
                            [[maybe_unused]] std::string& err 

namespace inb_impls
{
    void msg(INB_IMPL_PARAMETERS);
    void kill(INB_IMPL_PARAMETERS);

    inline std::unordered_map<std::string, void(*)(INB_IMPL_PARAMETERS)> INB_IMPLS_MAP = 
    {
        {"msg", msg},
        {"kill", kill}
    };
}