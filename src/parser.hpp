#pragma once

#include "token.hpp"
#include "rbc.hpp"
#include "lang.hpp"

#define ABORT_PARSE throw program.context
#define COMP_ERROR(_ec, message, ...)                                    \
    {                                                                    \
        *err = rs_error(message, content, currentToken->trace, fName, ##__VA_ARGS__);  \
        err->trace.ec = _ec;                                             \
        ABORT_PARSE;                                                     \
    }
#define COMP_ERROR_R(_ec, message, ret, ...)                             \
    {                                                                    \
        *err = rs_error(message, content, currentToken->trace, fName, ##__VA_ARGS__);  \
        err->trace.ec = _ec;                                             \
        ABORT_PARSE;                                                     \
    }

struct rbc_parser_flags
{
    bool parsingelif = false;
};

struct rbc_parser
{
    token_list&      tokens;

    size_t           _At = 0;
    size_t             S = tokens.size();

    std::string&     content;
    rbc_program      program;
    std::string      fName;
    rbc_parser_flags flags;

    std::shared_ptr<rs_error> err;

    token* currentToken;
    rbc_parser(token_list& _tokens, std::string fName, std::string& _content, std::shared_ptr<rs_error> _err) :
        tokens(_tokens),
        content(_content),
        program(_err.get()),
        err(_err)
    {
        _err->fName = fName;
    }

#pragma region lang
    bst_operation<token>       make_bst       (bool br = false,
                                               bool oneNode = false,
                                               bool obj = false);
    void                       prune_expr     (bst_operation<token>& expr);
    std::shared_ptr<rs_list>   parselist      ();
    rs_expression              expreval       (bool br = false,
                                               bool lineEnd = true,
                                               bool obj = false,
                                               bool prune = true);
                                   
    std::shared_ptr<rs_object> inlineobjparse ();
#pragma endregion lang

    void parseCurrent ();

    bool   resync  ();
    bool   match   (token_type tt, int info = -1);
    token* adv     (int i = 1);
    token* peek    (int x = 1);
    token* follows (token_type tt, int info = -1);

    #define RS_PARSER_VARIABLE_USE_CASE 0
    #define RS_PARSER_RETURN_USE_CASE 1
    #define RS_PARSER_LIST_ITEM_USE_CASE 2
    // verifies t == val or t can convert to val, throws an error otherwise
    // use case is used to give accurate descriptions to the errors.
    bool         typeverify (rs_type_info& t, rbc_value& val, int useCase);
    // parses a type
    rs_type_info typeparse  ();
    
    bool         callparse  (std::string& name, bool needsTermination = true, std::shared_ptr<rs_module> fromModule = nullptr);
    
    std::shared_ptr<rs_variable>
                 varparse   (token& name, bool needsTermination = true, bool parameter = false, bool obj = false, bool isConst = false);

    std::shared_ptr<rs_object>
                 objparse   (std::string& name);

    bool parsemoduleusage   (std::shared_ptr<rs_module> currentModule);

};