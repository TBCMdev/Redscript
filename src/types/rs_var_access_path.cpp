#include "rs_var_access_path.hpp"
#include "rs_variable.hpp"

#include "../globals.hpp"
#include "../mchelpers.hpp"
std::string rs_var_access_path::toPath() const
{
    std::stringstream stream;
    stream << fromVar->name;
    for(auto& p : segments)
    {
        std::string val;

        switch(p.accessKey.index())
        {
            case 0:
                val   = std::get<0>(p.accessKey).val;
                break;
            case 1:
            {
                rs_variable& var = *std::get<1>(p.accessKey);
                val = MC_VARIABLE_VALUE(var);
                break;
            }
            case 2:
                val = std::get<2>(p.accessKey)->toPath();
                break;
        }

        if (p.isArray)
            stream << '[' << val << ']';
        else
            stream << '.' << val;
    }

    return stream.str();
}
// "Can only be called if the variable associated with this path has been given a compilation index."
std::string rs_var_access_path::toCompiledPath() const
{
    std::stringstream stream;
    auto& fromvar_val = *fromVar;
    stream << getVariableValueLocation(fromvar_val);
    // if (!fromvar_val.comp_info.isParameter)
        // stream << '[' << fromvar_val.comp_info.varIndex << "].value";
    for(auto& p : segments)
    {
        std::string val;

        bool macro = true;

        switch(p.accessKey.index())
        {
            case 0:
                val   = std::get<0>(p.accessKey).val;
                macro = false;
                break;
            case 1:
            {
                rs_variable& var = *std::get<1>(p.accessKey);
                val = STR(var.comp_info.varIndex);
                break;
            }
            case 2:
                WARN("Cannot pass variable access eg x[0] to macro operator.");
                val = std::get<2>(p.accessKey)->toPath();
                break;
        }

        if (macro)
            val = "$(" + val + ')';

        if (p.isArray)
            stream << '[' << val << ']';
        else
            stream << '.' << val;
    }

    return stream.str();
}