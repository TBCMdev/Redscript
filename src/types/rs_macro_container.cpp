#include "rs_macro_container.hpp"
#include "../mchelpers.hpp"
std::string rs_macro_container::constructMacroPayload() const
{
    std::stringstream ret;
    auto& v = *macros.begin();

    rs_variable& var = *v.first;

    // we need to fix this asap
    ret << "storage ";

    ret << RS_PROGRAM_STORAGE SEP RS_PROGRAM_PARAMETERS_SPECIFIC(var.comp_info.get_stackframe_id());


    return ret.str();
}