#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <sstream>

#include "rs_variable.hpp"
#include "rbc_value.hpp"

enum class rs_object_member_decorator
{
    OPTIONAL,
    REQUIRED,
    SEPERATE,
};
struct rs_object
{
    const static uint32_t TYPE_CARET_START = 10; // custom types start at 10 and onwards for type id.
    using _MemberT = std::pair<std::shared_ptr<rs_variable>, rs_object_member_decorator>;
    // x.name -> 
    std::string name; // empty for inline objects
    int32_t scope;
    // negative for inline created objects
    int32_t typeID = -1;
    
    std::vector<std::shared_ptr<_MemberT>> members = {};
    std::unordered_map<std::string, std::shared_ptr<_MemberT>> namedMembers = {}; 
    inline std::string tostr()
    {
        std::stringstream stream;

        stream << "(obj)" << (name.empty() ? "{inline=1" : "{name=" + name) << ", members={";
        int i = 0;
        for(auto& member : members)
        {
            if (i != 0)
                stream << ',';
            stream << member->first->name;
            i++;
        }
        stream << "}}";
        return stream.str();
    }
    inline bool hasMember(const std::string& name)
    {
        return namedMembers.find(name) != namedMembers.end();
    }
    inline std::shared_ptr<_MemberT> getMember(const std::string& name)
    {
        auto f = namedMembers.find(name);
        
        if (f != namedMembers.end()) return f->second;

        return nullptr;
    }
    inline void addMember(const _MemberT& member)
    {
        auto shared = std::make_shared<_MemberT>(member);
        if (member.second != rs_object_member_decorator::SEPERATE) // cannot be accessed by index.
            members.push_back(shared);
        namedMembers.insert({member.first->name, shared});
    }
    void generateDefaultValues(std::unordered_map<std::string, rbc_value>& map);
    
};
struct rs_object_instance
{
    std::shared_ptr<rs_object> fromSchema = nullptr;
    
    std::unordered_map<std::string, rbc_value> values;

    rs_object_instance(std::shared_ptr<rs_object> ptr, std::unordered_map<std::string, rbc_value>& _values)
        : fromSchema(ptr), values(_values)
    { }
    rs_object_instance(std::shared_ptr<rs_object> ptr)
        : fromSchema(ptr)
    {
        ptr->generateDefaultValues(values);
    }
    rs_object_instance() : fromSchema(nullptr)
    { }
    inline std::string tostr()
    {
        return fromSchema->tostr();
    }
};
