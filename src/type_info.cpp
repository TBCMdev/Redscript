#include "type_info.hpp"

#include "parser.hpp"

std::string rs_type_info::find_generic_type_name() const
{
    if (!generic || generic_id == -1)
        return "unknown";

    std::vector<rs_type_info>* generics;
    if (!RS_PARSER_INSTANCE || (generics = &RS_PARSER_INSTANCE->program.genericTypeConversions)->empty())
        return "unknown";

    auto& t = generics->at(generic_id);
    if (t.generic)
        return "unknown";
        
    return t.type_name();
}