#pragma once

#include "token.hpp"
#include "rbc.hpp"
#include "lang.hpp"

struct rbc_parser_flags
{
    bool parsingelif = false;
};

struct rbc_parser
{
    token_list& tokens;

    size_t         _At = 0;
    size_t           S = tokens.size();

    rbc_program      program;
    std::string&     content;
    std::string      fName;
    rbc_parser_flags flags;

    std::shared_ptr<rs_error> err;

    token* currentToken;

    rbc_parser(token_list& _tokens, std::string& _content, rs_error* err) :
        tokens(_tokens),
        content(_content),
        program(err)
    {}

    void parseNext ();

    bool   resync  ();
    token* adv     (int i = 1);
    token* peek    (int x = 1);
    token* match   (token_type tt, int info = -1);
    token* follows (token_type tt, int info = -1);

    #define RS_PARSER_VARIABLE_USE_CASE 0
    #define RS_PARSER_RETURN_USE_CASE 1
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