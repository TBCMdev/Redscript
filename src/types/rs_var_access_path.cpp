#include "rs_var_access_path.hpp"
#include "rs_variable.hpp"

#include "../globals.hpp"
#include "../mchelpers.hpp"
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
// "Can only be called if the variable associated with this path has been given a compilation index."
std::string rs_var_access_path::toCompiledPath()
{
    std::stringstream stream;
    stream << RS_STORAGE_LOCATOR(*fromVar) << '[' << fromVar->comp_info.varIndex << "].value";
    for(auto& p : segments)
    {
        if (p.isArray)
            stream << '[' << p.accessKey.val << ']';
        else
            stream << '.' << p.accessKey.val;
    }

    return stream.str();
}