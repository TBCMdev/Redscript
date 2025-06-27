#include "parser.hpp"

#define ABORT_PARSE throw program.context

#define COMP_ERROR(_ec, message, ...)                                    \
    {                                                                    \
        *err = rs_error(message, content, currentToken->trace, fName, currentToken->trace.start, ##__VA_ARGS__);  \
        err->trace.ec = _ec;                                             \
        ABORT_PARSE;                                                     \
    }
#define COMP_ERROR_R(_ec, message, ret, ...)                             \
    {                                                                    \
        *err = rs_error(message, content, currentToken->trace, fName, currentToken->trace.start, ##__VA_ARGS__);  \
        err->trace.ec = _ec;                                             \
        ABORT_PARSE;                                                     \
    }
// todo make init function
void rbc_parser::parseNext()
{
    switch(currentToken->type)
    {
    case token_type::WORD:
    {
    // _parseword:
        token& word = *currentToken;
        if (follows(token_type::SYMBOL))
        {
            if(program.currentModule && !program.currentFunction)
                COMP_ERROR(RS_SYNTAX_ERROR, "Modules can only contain functions.");
            token* before = peek(-2);
            if (!varparse(word, 
                true,
                false,
                false,
                before && before->type == token_type::KW_CONST))
                ABORT_PARSE;
        }
        else if (follows(token_type::BRACKET_OPEN))
        {
            if(!callparse(word.repr, true, nullptr))
                ABORT_PARSE;
        }
        else if (follows(token_type::MODULE_ACCESS))
        {
            auto _module = program.modules.find(word);
            if (_module == program.modules.end())
                COMP_ERROR(RS_SYNTAX_ERROR, "Unknown module name.");

            if (!parsemoduleusage(_module->second))
                ABORT_PARSE;
        }
        break;
    }
    case token_type::MODULE_ACCESS:
    {
        // using current module access operator (::dostuff)
        if (program.currentModule)
        {
            if(!parsemoduleusage(program.currentModule))
                ABORT_PARSE;
        }
        break;
    }
    case token_type::KW_MODULE:
    {
        if(!adv())
            COMP_ERROR(RS_EOF_ERROR, "Expected module, not EOF.");
        if(program.currentFunction)
            COMP_ERROR(RS_SYNTAX_ERROR, "Modules are not allowed in a function body.");
        std::string name = currentToken->repr;
        std::vector<std::string> modulePath;
        if (program.currentModule)
        {
            auto& functions = program.currentModule->functions;
            if (functions.find(name) != functions.end())
                COMP_ERROR(RS_SYNTAX_ERROR, "Module already exists with that name.");
            modulePath = program.currentModule->modulePath;
            program.moduleStack.push(program.currentModule);
        }
        else if(program.modules.find(name) != program.modules.end())
            COMP_ERROR(RS_SYNTAX_ERROR, "Module already exists with that name.");

        auto val = program.modules.insert({name, std::make_shared<rs_module>()});
        program.currentModule = val.first->second;
        program.currentModule->name = name;

        modulePath.push_back(name);
        program.currentModule->modulePath = modulePath;
        program.scopeStack.push(rbc_scope_type::MODULE);



        if(!adv() || currentToken->type != token_type::CBRACKET_OPEN)
            COMP_ERROR(RS_EOF_ERROR, "Expected opening parenthesis.");
        // don't change scope!
        break;
    }
    case token_type::KW_METHOD:
    {
#pragma region function_definitions
        if (!adv())
            COMP_ERROR(RS_EOF_ERROR, "Expected name, not EOF.");
        
        rs_type_info retType;
        std::string name;
        if (currentToken->type != token_type::SYMBOL || currentToken->info != ':')
            COMP_ERROR(RS_SYNTAX_ERROR, "Missing function return type.");

        // we are defining the return type
        retType = typeparse();
        if (err->trace.ec)
            ABORT_PARSE;
        if(retType.equals(RS_NULL_KW_ID))
            COMP_ERROR(RS_SYNTAX_ERROR, "A functions' return type cannot be marked as null, use 'void' instead.");            
        name = currentToken->repr;
        
        if(currentToken->type == token_type::WORD)
            name = currentToken->repr;
        else
            COMP_ERROR(RS_SYNTAX_ERROR, "Invalid function name.");
        
        if (program.currentModule)
        {
            auto& functions = program.currentModule->functions;
            if (functions.find(name) != functions.end())
                COMP_ERROR(RS_SYNTAX_ERROR, "Function already exists in module.");
        }
        else if (program.functions.find(name) != program.functions.end())
            COMP_ERROR(RS_SYNTAX_ERROR, "Function already exists.");


        if (!adv())
            COMP_ERROR(RS_SYNTAX_ERROR, "Expected function definition, not EOF.");  
        
        if(currentToken->type != token_type::BRACKET_OPEN)
            COMP_ERROR(RS_SYNTAX_ERROR, "Expected '('.");
        
        for(char c : name)
        {
            if (std::isupper(c))
                COMP_ERROR(RS_SYNTAX_ERROR, "Function names cannot have uppercase letters due to how minecraft functions are implemented. Only use lower case letters and underscores.");
        }
        if (program.currentFunction)
            program.functionStack.push(program.currentFunction);

        program.currentFunction = std::make_shared<rbc_function>(name);
        program.currentFunction->scope = program.currentScope;
        program.currentFunction->returnType = std::make_shared<rs_type_info>(retType);
        program.scopeStack.push(rbc_scope_type::FUNCTION);

        if (program.currentModule)
            program.currentFunction->modulePath = program.currentModule->modulePath;

        program.currentScope ++;

        do
        {
            adv();
            switch(currentToken->type)
            {
                case token_type::BRACKET_CLOSED:
                    goto after_param;
                case token_type::WORD:
                {
                    token& varName = *currentToken;
                    if (!adv())
                        COMP_ERROR(RS_EOF_ERROR, "Unexpected EOF.");
                    // false as we do not need to terminate variable usage with ; or =
                    if (!varparse(varName, false, true)) // varparse adds any instructions to program.currentFunction
                        ABORT_PARSE;
                    if (currentToken->type == token_type::BRACKET_CLOSED)
                        goto after_param;
                    break;
                }
                default:
                    COMP_ERROR(RS_SYNTAX_ERROR, "Unexpected token.");
            }
        }
        while(currentToken->type != token_type::BRACKET_CLOSED);
            
    after_param:
        if(_At >= S)
            COMP_ERROR(RS_SYNTAX_ERROR, "Missing function definition or semi-colon.");
        
        while (adv() && currentToken->type == token_type::WORD)
        {
            const std::string& dName = currentToken->repr;

            auto decorator = parseDecorator(dName);

            if(decorator == rbc_function_decorator::UNKNOWN)
                COMP_ERROR(RS_SYNTAX_ERROR, "Unknown function decorator: '{}'.", dName);

            auto& decorators = program.currentFunction->decorators;
            if (std::find(decorators.begin(), decorators.end(), decorator) != decorators.end())
                COMP_ERROR(RS_SYNTAX_ERROR, "Duplicate function decorator: '{}'.", dName);

            decorators.push_back(decorator);
        }
        if(_At >= S)
            COMP_ERROR(RS_SYNTAX_ERROR, "Missing function definition or semi-colon.");
        switch(currentToken->type)
        {
            case token_type::CBRACKET_OPEN:
            {
                // break, hand over parsing to main loop, scope is incremented earlier for parameters. it will decrement when it reaches
                // a closing curly brace.
                break;
            }
            case token_type::LINE_END:
            {
                program.currentFunction->hasBody = false;
                goto _decrement_scope;
            }
            default:
                COMP_ERROR(RS_SYNTAX_ERROR, "Expected function definition or semi-colon.");
        }
        break;
#pragma endregion
    }
    case token_type::CBRACKET_CLOSED:
    {
    _decrement_scope:
        if(program.scopeStack.empty())
            COMP_ERROR(RS_SYNTAX_ERROR, "Unmatched closing bracket.");
        rbc_scope_type scope = program.scopeStack.top();
        program.scopeStack.pop();
        program.lastScope = scope;
        switch(scope)
        {
            case rbc_scope_type::MODULE:
            {
                if(program.moduleStack.size() > 0)
                {
                    std::shared_ptr<rs_module> child = program.currentModule;
                    
                    program.currentModule = program.moduleStack.top();
                    program.moduleStack.pop();

                    program.currentModule->children.insert({child->name, child});
                }
                else
                    program.currentModule = nullptr;
                break;
            }
            case rbc_scope_type::FUNCTION:
            {
                if (!program.currentFunction)
                    COMP_ERROR(RS_SYNTAX_ERROR, "Unexpected token.");

                // TODO: should functions have global scope access? if so, we need to keep track of when they are created.
                // I think not.

                if (!program.functionStack.empty())
                {
                    auto& parent = program.functionStack.top();

                    program.currentFunction->parent = parent;
                    parent->childFunctions.insert({program.currentFunction->name, program.currentFunction});
                    program.currentFunction = parent;

                    program.functionStack.pop();
                    break;
                }


                if (program.currentModule)
                    program.currentModule->functions.insert({program.currentFunction->name, program.currentFunction});
                else
                    program.functions.insert({program.currentFunction->name, program.currentFunction});

                program.currentFunction.reset();
                break;
            }
            case rbc_scope_type::IF:
            case rbc_scope_type::ELIF:
            {
                token* next = peek();
                if (!next || (next->type != token_type::KW_ELSE && next->type != token_type::KW_ELIF))
                    program(rbc_command(rbc_instruction::ENDIF));
                break;
            }
            case rbc_scope_type::ELSE:
            {
                token* next = peek();
                if (next && (next->type == token_type::KW_ELSE || next->type == token_type::KW_ELIF))
                    COMP_ERROR(RS_SYNTAX_ERROR, "Else & Elif blocks cannot follow an else block.");
                program(rbc_command(rbc_instruction::ENDIF));
                break;
            }
            case rbc_scope_type::NONE:
            {
                program(rbc_command(rbc_instruction::DEC));
                break;
            }
        }
        if (!program.currentModule)
            program.currentScope--;
        if (program.currentScope < 0)
            COMP_ERROR(RS_SYNTAX_ERROR, "Unmatched closing bracket.");
        break;
    }
    case token_type::CBRACKET_OPEN:
    {
        program.scopeStack.push(rbc_scope_type::NONE);
        program(rbc_command(rbc_instruction::INC));
        program.currentScope ++;
        break;
    }
    case token_type::KW_RETURN:
    {
        if (!program.currentFunction)
            COMP_ERROR(RS_SYNTAX_ERROR, "Return statements can only exist inside a function.");
        
        if (!adv())
            COMP_ERROR(RS_SYNTAX_ERROR, "Expected expression.");
        
        if (currentToken->type == token_type::LINE_END)
        {
            if (!program.currentFunction->returnType->equals(RS_VOID_KW_ID))
                COMP_ERROR(RS_SYNTAX_ERROR, "Cannot return nothing to a function with a return type of non-void.");
            program(rbc_command(rbc_instruction::RET));
            break;
        }

        // possible expression OR function call.
        token& start = *currentToken;

        if(follows(token_type::BRACKET_OPEN))
        {
            // function call TODO
            if(!callparse(start.repr, true, nullptr))
                ABORT_PARSE;
        }
        else
        {
            rs_expression expr = expreval(program, tokens, _At, err);
            if(err->trace.ec)
                ABORT_PARSE;
            rbc_value result = expr.rbc_evaluate(program, err); // evaluate and compute return statement
            if(err->trace.ec)
                ABORT_PARSE;
            if (!typeverify(*program.currentFunction->returnType, result, 1))
                ABORT_PARSE;
            program(rbc_command(rbc_instruction::RET, result));
        }
        break;
    }
    case token_type::KW_IF:
    {
    _parseif:
        if (!adv() || currentToken->type != token_type::BRACKET_OPEN)
            COMP_ERROR(RS_SYNTAX_ERROR, "Unexpected token.");
            
    _parseifagain: // for and and or keywords
        if(!adv())
            COMP_ERROR(RS_SYNTAX_ERROR, "Expected expression, not EOF.");
        // br = true as if we hit the closing bracket of the if statement we should return.
        rs_expression left = expreval(program, tokens, _At, err, true, false, false);
        if(err->trace.ec)
            ABORT_PARSE;
        rbc_value lVal = left.rbc_evaluate(program, err);
        if(err->trace.ec)
            ABORT_PARSE;
        
        resync();

        if (currentToken->type == token_type::BRACKET_CLOSED)
        {
            program(rbc_command(flags.parsingelif ? rbc_instruction::ELIF : rbc_instruction::IF, lVal));
        end_if_parse:
            if (!adv())
                COMP_ERROR(RS_EOF_ERROR, "Unexpected EOF.");

            if (currentToken->type != token_type::CBRACKET_OPEN)
                COMP_ERROR(RS_SYNTAX_ERROR, "Unexpected token.");
            
            program.currentScope++;
            program.scopeStack.push(flags.parsingelif ? rbc_scope_type::ELIF : rbc_scope_type::IF);
            break;
        }

        token& op         = *currentToken;
        token_type compop = op.type;   
        if(compop != token_type::COMPARE_EQUAL && compop != token_type::COMPARE_NOTEQUAL)
            COMP_ERROR(RS_SYNTAX_ERROR, "Unexpected token.");

        if(!adv())
            COMP_ERROR(RS_SYNTAX_ERROR, "Expected expression, not EOF.");
        // br = true as if we hit the closing bracket of the if statement we should return.
        // prune = false as expreval has mismatching problems after when encountering this ).
        // horrible code having 4 boolean flags, maybe make struct.
        rs_expression right = expreval(program, tokens, _At, err, true, false, false);
        if (err->trace.ec)
            ABORT_PARSE;
        resync();
        rbc_value rVal = right.rbc_evaluate(program, err);
        if(err->trace.ec)
            ABORT_PARSE;
        token_type t = currentToken->type;

        switch(t)
        {
            case token_type::KW_AND:
            case token_type::KW_OR:
                if(!adv())
                    COMP_ERROR(RS_SYNTAX_ERROR, "Expected expression, not EOF.");
                goto _parseifagain;
            case token_type::BRACKET_CLOSED:
                break;
            default:
                COMP_ERROR(RS_SYNTAX_ERROR, "Unexpected token.");

        }
        program(rbc_command(flags.parsingelif ? rbc_instruction::ELIF : rbc_instruction::IF, lVal, rbc_constant(compop, op.repr, &op.trace), rVal));
        
        goto end_if_parse;
    }
    case token_type::KW_ELIF:
    {

        if (program.lastScope != rbc_scope_type::IF && program.lastScope != rbc_scope_type::ELIF)
            COMP_ERROR(RS_SYNTAX_ERROR, "elif blocks can only be used after an if block.");
        flags.parsingelif = true;
        goto _parseif;
    }
    case token_type::KW_ELSE:
    {
        if (program.lastScope != rbc_scope_type::IF && program.lastScope != rbc_scope_type::ELIF)
            COMP_ERROR(RS_SYNTAX_ERROR, "Else and elif blocks can only be used after an if block.");
        

        if (!adv() || currentToken->type != token_type::CBRACKET_OPEN)
            COMP_ERROR(RS_SYNTAX_ERROR, "Expected else block.");
        
        program.scopeStack.push(rbc_scope_type::ELSE);
        program(rbc_command(rbc_instruction::ELSE));
        program.currentScope++;
        break;
    }
    case token_type::TYPE_DEF:
    {
        switch (currentToken->info)
        {
            case RS_OBJECT_KW_ID:
            {
                if (!adv())
                    COMP_ERROR(RS_EOF_ERROR, "Unexpected EOF.");
                
                std::string& name = currentToken->repr;

                if (currentToken->type != token_type::WORD)
                    COMP_ERROR(RS_SYNTAX_ERROR, "Unexpected keyword.");

                if (program.objectTypes.find(name) != program.objectTypes.end())
                    COMP_ERROR(RS_SYNTAX_ERROR, "Object with name already exists.");
                
                adv();

                auto obj = objparse(name);
                if (err->trace.ec)
                    ABORT_PARSE;
                obj->typeID = rs_object::TYPE_CARET_START + program.objectTypes.size();
                program.objectTypes.insert({name, obj});
                break;
            }
            default:
                WARN("Unimplemented type.");
                break;
        }
        
        break;
    }
    default:
        WARN("Found suspicious token (possible due to a non implemented feature).");
        break;
    }
}