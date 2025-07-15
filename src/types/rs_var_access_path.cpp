#include "rs_var_access_path.hpp"
#include "rs_variable.hpp"

#include "../globals.hpp"

std::string rs_var_access_path::toPath()
{
    std::stringstream stream;
    stream << fromVar->name;
    for(auto& p : segments)
    {
        if (p.isArray)
            stream << '[' << p.accessKey.val << ']';
        else
            stream << '.' << p.accessKey.val;
    }

    return stream.str();
}
__warnattr("Can only be called if the variable associated with this path has been given a compilation index.")
std::string rs_var_access_path::toCompiledPath()
{
    std::stringstream stream;
    stream << RS_PROGRAM_VARIABLES << '[' << fromVar->comp_info.varIndex << ']';
    for(auto& p : segments)
    {
        if (p.isArray)
            stream << '[' << p.accessKey.val << ']';
        else
            stream << '.' << p.accessKey.val;
    }

    return stream.str() + ".value";
}