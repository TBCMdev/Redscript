#include "rs_object.hpp"
#include "rs_list.hpp"
#include "../typeinfo"
void rs_object::generateDefaultValues(std::unordered_map<std::string, rbc_value>& map)
{
    std::vector<rs_object_instance*> copies;
    for(auto& [key, val] : namedMembers)
    {
        auto& var = *val->first;
        
        if (var.value) continue;

        rs_type_info& t = var.type_info;
        if (t.isFinallyOptional())
            map.insert({key, rbc_constant(token_type::INT_LITERAL, "0")});
        else if (t.array_count > 0)
            map.insert({key, std::make_shared<rs_list>()});
        else if (t.fromObject)
        {
            if (t.fromObject.get() == this)
            {
                auto instance = std::make_shared<rs_object_instance>();
                instance->fromSchema = t.fromObject; // avoid constructor
                copies.push_back(instance.get());

                map.insert({key, instance});
            }
            else
                map.insert({key, std::make_shared<rs_object_instance>(t.fromObject)});
        }
        else
        {
            switch(t.type_id)
            {
                case RS_INT_KW_ID:
                case RS_BOOL_KW_ID:
                    map.insert({key, rbc_constant(token_type::INT_LITERAL, "0")});
                    break;
                case RS_STRING_KW_ID:
                    map.insert({key, rbc_constant(token_type::STRING_LITERAL, "")});
                    break;
                case RS_FLOAT_KW_ID:
                    map.insert({key, rbc_constant(token_type::FLOAT_LITERAL, "0F")});
                    break;
                default:
                    WARN("Unknown default constant of type_id: %d", t.type_id);
            }
        }
    }
    // transfer map data to copies, todo make into ptr? saves memory
    for(auto& schemaCopy : copies)
        schemaCopy->values = map;
}