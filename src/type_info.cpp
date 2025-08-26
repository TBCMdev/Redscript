#include "type_info.hpp"

#include "types/rs_object.hpp"
#include "parser.hpp"
std::string rs_type_info::find_generic_type_name() const
{
    if (!generic || generic_id == -1)
        return "generic";

    std::vector<rs_type_info>* generics;
    if (!RS_PARSER_INSTANCE || (generics = &RS_PARSER_INSTANCE->program.genericTypeConversions)->empty())
        return "generic";

    auto& t = generics->at(generic_id);
    if (t.generic)
        return "generic";
        
    return t.type_name();
}
std::string rs_type_info::type_name() const
{
    if (generic)
        return find_generic_type_name();
    if (type_id != -1 && fromObject)
        return fromObject->name;
    switch(type_id)
    {
        case RS_INT_KW_ID:
            return "int";
        case RS_STRING_KW_ID:
            return "string";
        case RS_FLOAT_KW_ID:
            return "float";
        case RS_BOOL_KW_ID:
            return "bool";
        case RS_OBJECT_KW_ID:
            return "object";
        default:
            return "unknown";
    }
}