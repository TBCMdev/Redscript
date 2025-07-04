#pragma once
#include <cstdint>
#include <string>
#include <variant>
#include <unordered_map>

#include "globals.hpp"
#include "constants.hpp"

struct rbc_program;
struct rbc_register;
struct rbc_constant;

struct rs_variable;
struct rs_object;
struct rs_list;
#include "bst.hpp"
#include "token.hpp"
#include "type_info.hpp"
// holds a bst for pruning and computing,
// or a shared ptr to a raw non operational result such as an object.
// this ptr is not given a value to singleton expressions, such as integers or strings.
// only to values that cannot be operated on.
struct rs_expression
{
    using _ResultT = RBC_VALUE_T;
    bst_operation<token> operation;
    std::shared_ptr<_ResultT> nonOperationalResult = nullptr;
    
    _ResultT rbc_evaluate(rbc_program&,
                               bst_operation<token>* = nullptr);
};
struct rs_compilation_info
{
    int varIndex = 0;
};
class rs_variable
{
public:
    token&       from;
    std::string  name;
    int32_t     scope;
    rs_type_info type_info, real_type_info;
    bool global = false; bool _const = false; bool ref = false;

    std::shared_ptr<rs_expression> value = nullptr;
    std::shared_ptr<rs_object> fromObject = nullptr;

    rs_compilation_info comp_info;

    rs_variable(token& _from, uint32_t _scope = 0, bool _global = false)
        : from(_from), name(_from.repr), scope(_scope), global(_global)
    {}
    rs_variable(token& _from, rs_type_info realInfo, int32_t _scope = 0, bool _global = false)
        : from(_from), name(_from.repr), scope(_scope), real_type_info(realInfo), global(_global)
    {}
    rs_variable(token& _from, rs_type_info explicitInfo, rs_type_info realInfo, int32_t _scope = 0, bool _global = false)
        : from(_from), name(_from.repr), scope(_scope), type_info(explicitInfo), real_type_info(realInfo), global(_global)
    {}

    inline std::string tostr()
    {
        std::stringstream stream;

        stream << '{' << from.str() << ", name=" << name << ", scope=" << scope << ", typeinf=" << type_info.tostr() << ", (g=" << global << ", c=" << _const << ")}";
        return stream.str();
    }
};
enum class rs_object_member_decorator
{
    OPTIONAL,
    REQUIRED,
    SEPERATE,
};
struct rs_object
{
    const static uint32_t TYPE_CARET_START = 10; // custom types start at 10 and onwards for type id.
    using _MemberT = std::pair<rs_variable, rs_object_member_decorator>;
    // x.name -> 
    std::string name; // empty for inline objects
    int32_t scope;
    // negative for inline created objects
    int32_t typeID = -1;
    std::unordered_map<std::string, _MemberT> members = {};

    inline std::string tostr()
    {
        std::stringstream stream;

        stream << "(obj)" << (name.empty() ? "{inline=1" : "{name=" + name) << ", members={";
        int i = 0;
        for(auto& member : members)
        {
            if (i != 0)
                stream << ',';
            stream << member.second.first.name;
            i++;
        }
        stream << "}}";
        return stream.str();
    }
};

struct rs_list
{
    rs_type_info elementType;
    std::vector<std::shared_ptr<RBC_VALUE_T>> values;
};

void prune_expr(rbc_program&, bst_operation<token>&, rs_error*);
