#include "rs_variable.hpp"
#include "rs_compilation_info.hpp"
#include "../mchelpers.hpp"

std::string getVariableValueLocation(const rs_variable& var)
{
    const short stackID = var.comp_info.belongingStackFrame ? var.comp_info.get_stackframe_id() : 0;
    if (var.comp_info.isParameter)
        return RS_PROGRAM_PARAMETERS_SPECIFIC(stackID) "." + VAR_ID(var);
    
    return ARR_AT(RS_PROGRAM_VARIABLES_SPECIFIC(stackID), VAR_ID(var)) + ".value";
}