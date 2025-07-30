#include "parser.hpp"

#include "types/rs_object.hpp"
#include "types/rbc_constant.hpp"
#include "types/rbc_register.hpp"
#include "types/rs_list.hpp"
#include "types/rs_variable.hpp"

#include "types/rbc_value.hpp"

#include "types/rbc_types.hpp"

#pragma region lang

std::shared_ptr<rs_object>   rbc_parser::inlineobjparse   ()
{
    rs_object obj;
    while(adv())
    {
        token& name = *currentToken;
        if (name.type == token_type::CBRACKET_CLOSED)
            break;
        if (name.type != token_type::WORD)
            COMP_ERROR(RS_SYNTAX_ERROR, "Unexpected token.");
        if (obj.members.find(name) != obj.members.end())
            COMP_ERROR(RS_SYNTAX_ERROR, "Duplicate object field.");

        adv();
        token& sep = *currentToken;
        if (sep.type != token_type::SYMBOL || sep.info != ':')
            COMP_ERROR(RS_SYNTAX_ERROR, "Expected ':'.");

        adv();

        rs_expression value = expreval(false, false, true);
        if (value.nonOperationalResult)
            adv();

        rs_variable var(name, program.currentScope);
        var.value = std::make_shared<rs_expression>(value);
        obj.members.insert({name.repr, {var, rs_object_member_decorator::OPTIONAL}});

        token& terminator = *currentToken;
        if (terminator.type == token_type::CBRACKET_CLOSED)
            break;
        if (terminator.type != token_type::SYMBOL || terminator.info != ',')
            COMP_ERROR(RS_SYNTAX_ERROR, "Expected object field seperator or '}'.");
    }
    if (!peek())
        COMP_ERROR(RS_SYNTAX_ERROR, "Unterminated object definition.");

    return std::make_shared<rs_object>(obj);
    
}
// called at index of first access token
rs_var_access_path rbc_parser::parse_var_path_access(std::shared_ptr<rs_variable>& var)
{
    rs_var_access_path path{var};
    rs_type_info currentType = var->type_info;
    while(1)
    {
        if (equals(token_type::SQBRACKET_OPEN))
        {
            adv(); // for better errors
            if (equals(token_type::SQBRACKET_CLOSED))
                COMP_ERROR(RS_SYNTAX_ERROR, "Expected integer constant.");
            if (!equals(token_type::INT_LITERAL))
                COMP_ERROR(RS_SYNTAX_ERROR, "Indexing arrays can only be done with integer constants. use array::at() to access an array via variables or other.");

            if (currentType.array_count <= 0)
                COMP_ERROR(RS_SYNTAX_ERROR, "Cannot index value of non list type (evaldT={})", currentType.tostr());
            currentType = currentType.element_type();

            path.segments.push_back(rs_var_access_path_item{rbc_constant(token_type::INT_LITERAL, currentToken->repr), true});
            adv();

            expect(token_type::SQBRACKET_CLOSED);
        }
        else if (equals(token_type::OBJECT_ACCESS_OPERATOR))
        {
            COMP_ERROR(RS_UNSUPPORTED_OPERATION_ERROR, "Object accessing has yet to be implemented.");
        }
        else break;

        token* t;
        if ((t = peek()) && t->type == token_type::LINE_END)
            break;

        adv();
    }
    path.evaluatedType = currentType;
    return path;
}
bst_operation<token>         rbc_parser::make_bst         (bool br, bool oneNode, bool obj)
{
    bst_operation<token> root;
    do
    {
        switch (currentToken->type)
        {
        case token_type::LINE_END:
            return root;
        case token_type::BRACKET_CLOSED:
        {
            if (!br)
                COMP_ERROR(RS_SYNTAX_ERROR, "Unclosed bracket found.");
            if (!root.left->index() && !root.right)
            {
                auto& node = std::get<0>(*root.left);
                if (node.right && node.left->index() && node.right->index())
                {
                    // they are both tokens, and therefore dont need to be
                    // encapsulated.
                    root.right = node.right;
                    root.operation = node.operation;
                    root.left  = node.left;
                }
            }
            return root;
        }
        case token_type::BRACKET_OPEN:
        {
            adv();
            bst_operation<token> child = make_bst(true, false);

            if (!root.assignNext(child))
                COMP_ERROR(RS_SYNTAX_ERROR, "Missing operator.");
            // _At ++;
            if (oneNode)
                return root;
            break;
        }
        case token_type::OPERATOR:
        {
            if (root.operation != bst_operation_type::NONE)
                COMP_ERROR(RS_SYNTAX_ERROR, "Unexpected token.");
            if (currentToken->repr.length() > 1)
                COMP_ERROR(RS_SYNTAX_ERROR, "Unexpected operator.");
            const char op = currentToken->info;

            if (!root.setOperation(op))
                COMP_ERROR(RS_SYNTAX_ERROR, "Unsupported operator.");

            break;
        }
        case token_type::INT_LITERAL:
        case token_type::FLOAT_LITERAL:
        case token_type::STRING_LITERAL:
        case token_type::LIST_LITERAL:
        case token_type::OBJECT_LITERAL:
        case token_type::WORD:
        case token_type::KW_NULL:
        {
            if (currentToken->type == token_type::WORD)
            {
                sharedt<rs_variable> v;
                if (!(v = program.getVariable(currentToken->repr)))
                    COMP_ERROR(RS_SYNTAX_ERROR, "Unexpected token in expression.");

                if (!follows(token_type::SQBRACKET_OPEN) && !follows(token_type::OBJECT_ACCESS_OPERATOR))
                {
                    if (!root.assignNext(*currentToken))
                        COMP_ERROR(RS_SYNTAX_ERROR, "Missing operator.");
                }
                else
                {
                    rs_var_access_path path = parse_var_path_access(v);
                    path.fromVar = v;
                    if(!root.assignNext(path))
                        COMP_ERROR(RS_SYNTAX_ERROR, "Missing operator.");
                }    
            }
            else if (!root.assignNext(*currentToken))
                COMP_ERROR(RS_SYNTAX_ERROR, "Missing operator.");
            token* t = nullptr;
            if (oneNode || ((t = peek()) && t->type == token_type::LINE_END))
                return root;
            break;
        }
        default:
            // exceptions: these are all handled externally
            if ((currentToken->info == ',' || currentToken->type == token_type::SQBRACKET_CLOSED) ||
                (currentToken->info == '}' && obj)         ||
                (currentToken->type == token_type::COMPARE_EQUAL || currentToken->type == token_type::COMPARE_NOTEQUAL))
            {
                return root; // commas can end expressions
            }
            COMP_ERROR(RS_SYNTAX_ERROR, "Unknown token in expression.");
        }
        if (root.right)
        {
            token* next = nullptr;
            while ((next = peek()) && next->type == token_type::OPERATOR)
            {
                if (next->repr.length() != 1) // todo remove 
                    COMP_ERROR(RS_SYNTAX_ERROR, "Unsupported operator.");
                const int pLeft = operatorPrecedence(root.operation), pRight = operatorPrecedence(next->info);

                _At += 2;
                if (_At >= S)
                    COMP_ERROR(RS_SYNTAX_ERROR, "Expected expression, not EOF.");
                // check if we are in a bracket, and advance to next token
                bool isBracketNode = currentToken->type == token_type::BRACKET_OPEN;
                if (isBracketNode) adv();

                if (pRight < pLeft)
                {
                    // parse the next node only, concat root.right and new right to make more favorable node
                    bst_operation<token> right = make_bst(isBracketNode, true);
                    right.right = root.right;
                    right.setOperation(next->info);

                    std::swap(*right.left, *right.right);

                    root.right = std::make_shared<bst_operation<token>::_ValueT>(right);
                } else
                {
                    // we encapsulate root node with another node.
                    bst_operation<token> newRoot;
                    newRoot.setOperation(next->info);
                    // assign left
                    newRoot.assignNext(root);

                    bst_operation<token> right = make_bst(isBracketNode, true);
                    // assign right
                    newRoot.assignNext(right);

                    root = newRoot;
                }
                if(isBracketNode) _At++;
            }
            break;
        }
        
        if (resync() && currentToken->type == token_type::LINE_END)
            return root;
    } while (adv());

    if (!resync())
        COMP_ERROR(RS_EOF_ERROR, "Expected expression, not EOF.");

    return root;
}
void                         rbc_parser::prune_expr       (bst_operation<token>& expr)
{
    using _NodeT = bst_operation<token>;
    using _ValueT = bst_operation<token>::_NodeT;

    // declared here for lambda simplicity
    token result;


    bool lSingle  = expr.left->index();
    bool rSingle = expr.right ? expr.right->index() : false;
#pragma region expression_compute_methods
    const static auto _impl_computeint =
        [&](const token& left, const token& right)
        {
            int r  = operator_compute(std::stoi(left.repr), expr.operation, std::stoi(right.repr));
            result.repr = std::to_string(r);
        };
#pragma endregion expression_compute_methods
    if (lSingle && rSingle)
    {
        // compute
        _ValueT& lVariant = std::get<_ValueT>(*expr.left);
        _ValueT& rVariant = std::get<_ValueT>(*expr.right);

        // if one of the node values is accessing an array or object, just forget about it.
        // (im lazy)
        if (lVariant.index() || rVariant.index())
            return;

        token& left = std::get<token>(lVariant);
        token& right = std::get<token>(rVariant);
        if(left.type == right.type)
        {
            result = left; // doesnt matter left or right
            switch (left.type)
            {
                case token_type::INT_LITERAL:
                {
                    _impl_computeint(left, right);
                    break;
                }
                default:
                    WARN("No supported operation of same type (T=%s)", tutil::type_to_str(left.type).c_str());
                    break;
            }
        }
        else
        {

            // edge cases
            const static pairedkey_map<token_type, combination_functor<token>> map = 
            {
                // DEPRECATED -- TODO REMOVE -- NO MATH WITH NULL KEYWORD ALLOWED
                // because we are adding to null, we just return the number we are adding as null = 0
                // { {token_type::KW_NULL, token_type::INT_LITERAL} , [&result](UNUSED const token& lhs, const token& rhs)
                //     { 
                //         // must assign result a trace object, as well as type of rhs (int)
                //         result = rhs;
                //         result.repr = rhs.repr;
                //     }
                // }
            };

            bool found = combinationCommutativeEquals(left.type, right.type, left, right, map);

            if (!found)
            {
                std::string lType = tutil::type_to_str(left.type), rType = tutil::type_to_str(right.type);
                // for nice errors, check if the sides are variables and fetch their types
                if (left.type == token_type::WORD)
                {
                    auto var = program.getVariable(left.repr);
                    if (var)
                        lType = var->type_info.tostr();
                }
                if (right.type == token_type::WORD)
                {
                    auto var = program.getVariable(right.repr);
                    if (var)
                        rType = var->type_info.tostr();
                }

                WARN("No supported operation of different type (T=%s, T1=%s)",
                    lType.c_str(),
                    rType.c_str());
            }
        }
        if(!result.repr.empty())
            expr.makeSingular(result);
    }
    else
    {
        if(!lSingle)
        {
            _NodeT& lchild = std::get<_NodeT>(*expr.left);
            prune_expr(lchild);
            if (lchild.isSingular())
                expr.left = std::make_shared<_NodeT::_ValueT>(std::get<_ValueT>(*lchild.left));
        }
        if(!rSingle && expr.right)
        {
            _NodeT& rchild = std::get<_NodeT>(*expr.right);
            prune_expr(rchild);
            if (rchild.isSingular())
                expr.right = std::make_shared<_NodeT::_ValueT>(std::get<_ValueT>(*rchild.left));
        }
        if (expr.left->index() && expr.right && expr.right->index())
            prune_expr(expr);
    }
    
}
rs_expression                rbc_parser::expreval         (bool br, bool lineEnd, bool obj, bool prune)
{
    rs_expression expr;
    token& current = *currentToken;
    if (current.type == token_type::CBRACKET_OPEN)
    {
        // parse object
        std::shared_ptr<rs_object> obj = inlineobjparse();
        expr.nonOperationalResult = std::make_shared<rbc_value>(obj);
        return expr;
    }
    else if (current.type == token_type::SELECTOR_LITERAL)
    {
        // todo: make selector parse a function so that we can have complex selectors:
        // @p[name=x]
        expr.nonOperationalResult = std::make_shared<rbc_value>(rbc_constant(token_type::SELECTOR_LITERAL, current.repr));
        return expr;
    }
    else if (current.type == token_type::SQBRACKET_OPEN)
    {
        expr.nonOperationalResult = std::make_shared<rbc_value>(parselist());
        return expr;
    }
    bst_operation<token> bst = make_bst(br, false, obj);

    if (lineEnd)
    {
        if (!resync())
            COMP_ERROR(RS_EOF_ERROR, "Missing semicolon.");
        adv();
        expect(token_type::LINE_END);
    }
    else if (!bst.isSingular())
        adv();

    if(prune)
        prune_expr(bst);

    expr.operation = bst;

    return expr;
}

#pragma region lang

void                 rbc_parser::typeverify       (const rs_type_info& t, rbc_value& val, int useCase)
{
    switch(val.index())
    {
        case 0:
        {
            rbc_constant& c = std::get<0>(val);
            int x = static_cast<int>(c.val_type);
            if (!t.equals(x))
            {
                std::string error;
                switch(useCase)
                {
                    case 0: // variable assignment
                        error = "Cannot assign constant of type {} to variable of type {}.";
                        break;
                    case 1: // return
                        error = "Return expression evaluated type does not match function signature.";
                        break;
                    default:
                        error = "Evaluated type of constant ({}) cannot convert to expected type of {}.";
                }
                COMP_ERROR_T(RS_SYNTAX_ERROR, error, *c.trace, tutil::type_to_str(c.val_type), t.tostr());
            }
            break;
        }
        case 1:
        {
            rbc_register& reg = *std::get<1>(val);
            if (reg.operable)
            {
                if (t.equals(RS_INT_KW_ID)) break;
                std::string error;
                switch(useCase)
                {
                    case 0: // variable assignment
                        error = "Expected integer expression to assign to variable of type integer.";
                        break;
                    default:
                        error = "Evaluated type of this expression is not allowed here.";
                }
                COMP_ERROR_R(RS_SYNTAX_ERROR, error, false);
            } else COMP_ERROR_R(RS_UNSUPPORTED_OPERATION_ERROR, "Non-operable registers arent supported for typing.", false);

            break;
        }
        case 2:
        {
            rs_variable& var = *std::get<2>(val);
            if (!t.equals(var.type_info))
            {
                std::string error;
                switch(useCase)
                {
                    case 0: // variable assignment
                        error = "Cannot assign variable value of type {} to variable of different type {}.";
                        break;
                    case 1:
                        error = "Variable being returned has type (of {}) that does not match function return signature of {}.";
                        break;
                    default:
                        error = "Evaluated type of {} cannot convert to expected type of {}.";
                }
                COMP_ERROR(RS_SYNTAX_ERROR, error, var.type_info.tostr(), t.tostr());
            }
            break;
        }
        case 3:
        {
            WARN("No type checking for objects as of yet.");
            break;
        }
        case 4:
        {
            rs_list& list = *std::get<4>(val);
            if (t.array_count == 0)
            {
                // todo impl list errors
                std::string error;
                switch(useCase)
                {
                    case 0: // variable assignment
                        error = "Cannot assign variable value to variable of different type.";
                        break;
                    case 1:
                        error = "Return expression evaluated type does not match function signature.";
                        break;
                    default:
                        error = "List of child type {} cannot convert to expected type of {}.";
                }
                COMP_ERROR(RS_SYNTAX_ERROR, error, list.elementType.tostr(), t.tostr());
            }
            else 
            {
                auto elType = t.element_type();
                for (std::shared_ptr<rbc_value>& item : list.values)
                    typeverify(elType, *item, RS_PARSER_LIST_ITEM_USE_CASE);
            }
            break;
        }
    }
}
bool                         rbc_parser::resync           ()
{
    if (_At > S - 1) return false;

    currentToken = &tokens->at(_At);
    return true;
}
token*                       rbc_parser::s_adv            (int i)
{
   if (_At + i >= S)
    {
        if (files.empty())
            return nullptr;
        
        useNextFile();
    }
    else
        _At += i;

    currentToken = &tokens->at(_At);

    // stack trace needs to be updated here,
    // i.e all tokens should contain a stack trace within them.

    return currentToken; 
}
token*                       rbc_parser::adv              (int i)
{
    if (_At + i >= S)
    {
        if (files.empty())
            COMP_ERROR(RS_EOF_ERROR, "Unexpected EOF.");
        
        useNextFile();
    }
    else
        _At += i;

    currentToken = &tokens->at(_At);

    // stack trace needs to be updated here,
    // i.e all tokens should contain a stack trace within them.

    return currentToken;
};
token*                       rbc_parser::peek             (int x)
{
    if (_At + x >= S)
        return nullptr;

    token* t = &tokens->at(_At + x);

    // stack trace needs to be updated here,
    // i.e all tokens should contain a stack trace within them.

    return t;
};
bool                         rbc_parser::match             (token_type tt, int info)
{
    token* next = peek();
    if(next && next->type == tt)
    {
        if(info > -1 && next->info != info) return false;

        adv();
        return true;
    }

    return false;
};
bool                         rbc_parser::equals            (token_type tt, int info)
{
    if(currentToken && currentToken->type == tt)
    {
        if(info > -1 && currentToken->info != info) return false;

        return true;
    }

    return false;
};
void                         rbc_parser::expect            (token_type tt, std::string msg, int info)
{
    if (currentToken->type != tt)
    {
        std::string s = tutil::type_to_str_full(tt);
        COMP_ERROR(RS_SYNTAX_ERROR, msg, s);
    }
    if (info > -1 && currentToken->info != info)
    {
        std::string s = tutil::type_to_str_full(tt);
        COMP_ERROR(RS_SYNTAX_ERROR, msg, s, info);
    }
}

void                         rbc_parser::expect            (token_type tt, int info)
{
    if (currentToken->type != tt)
    {
        std::string s = tutil::type_to_str_full(tt);
        COMP_ERROR(RS_SYNTAX_ERROR, "Expected {}.", s);
    }
    if (info > -1 && currentToken->info != info)
    {
        std::string s = tutil::type_to_str_full(tt);
        COMP_ERROR(RS_SYNTAX_ERROR, "Expected special {} (id: {})", s, info);
    }
}
void                         rbc_parser::expect            (char info, token_type tt)
{
    if (currentToken->type != tt)
        COMP_ERROR(RS_SYNTAX_ERROR, "Expected '{}'.", info);
    if (info > -1 && currentToken->info != info)
        COMP_ERROR(RS_SYNTAX_ERROR, "Expected special '{}'.", info);
}
// bad impl may change
template<typename... _Infos>
void rbc_parser::expect_any(_Infos&&... chars)
{
    bool match_found = ((currentToken->info == chars && chars > -1) || ...);

    if (!match_found)
        COMP_ERROR(RS_SYNTAX_ERROR, "Unexpected token.");
}


void                         rbc_parser::nexpect            (token_type tt, int info)
{
    if (currentToken->type == tt)
    {
        std::string s = tutil::type_to_str_full(tt);
        COMP_ERROR(RS_SYNTAX_ERROR, "Unexpected token of type {}.", s);
    }
    if (info == -1 && currentToken->info == info)
    {
        std::string s = tutil::type_to_str_full(tt);
        COMP_ERROR(RS_SYNTAX_ERROR, "Unexpected specific token of type {} (id: {:c})", s, info);
    }
}
void                         rbc_parser::nexpect            (char info)
{
    if (info == -1 && currentToken->info == info)
    {
        std::string s = tutil::type_to_str_full(currentToken->type);
        COMP_ERROR(RS_SYNTAX_ERROR, "Unexpected specific token of type {} (id: {:c})", s, info);
    }
}
token*                       rbc_parser::follows           (token_type tt, int info)
{
    token* next = peek();
    if(next && next->type == tt)
    {
        if(info > -1 && next->info != info) return nullptr;
        adv();
        return next;
    }
    return nullptr;
};
std::shared_ptr<rs_list>     rbc_parser::parselist        ()
{
    rs_list list;
    do
    {
        adv();
        if (equals(token_type::SQBRACKET_CLOSED))
            break;
        rs_expression expr = expreval(false, false);
        rs_expression::_ResultT val = expr.rbc_evaluate(program);
        // a list expression's element type is determined by its first element. [PUT IN DOCS]
        if (list.elementType.type_id == -1)
            list.elementType = typeinfer(val);

        if (val.index() == 4) // a list which was parsed fine, so we are at a different ']'
            adv();
        list.values.push_back(std::make_shared<rbc_value>(val));
    } while (equals(token_type::SYMBOL, ','));

    expect(token_type::SQBRACKET_CLOSED);
    

    return std::make_shared<rs_list>(list);
}
rs_type_info                 rbc_parser::typeparse        ()
{

    rs_type_info tinfo;
    
    token* next = adv();

    int typeID     = next->info;
    int genericID  = -1;

    bool convertedFromGeneric = false;

    // its not any kw
    if(typeID == 0 && next->type != token_type::TYPE_DEF)
    {
        // TODO find type
        std::string& repr = next->repr;
        auto custom = program.objectTypes.find(repr);
        if (custom != program.objectTypes.end())
        {
            typeID = custom->second->typeID;
            goto _after_type;
        }

        auto generic = program.currentGenericTypes.find(repr);
        if (generic != program.currentGenericTypes.end())
        {
            int conversionID = generic->second.first;
            // see if we can convert it (we are parsing a generic function call).
            if (conversionID < (int)program.genericTypeConversions.size())
            {
                tinfo.assignType(program.genericTypeConversions.at(conversionID));
                convertedFromGeneric = true;
            }
            else
                genericID = generic->second.first;

            goto _after_type;
        }
        
        COMP_ERROR_R(RS_SYNTAX_ERROR, "Type name unknown or not supported.", tinfo);
    }
_after_type:
    next = peek();
    bool optional = false;
    bool reference = false;
    if(next)
    {
        if (next->info == '?')
        {
            optional = true;
            adv();
        }
        else if (next->info == '&')
        {
            reference   = true;
            adv();
        }
    }

    uint arrayCount = tinfo.array_count;
    std::vector<std::pair<bool, bool>> arrayFlags;
    while ((next = follows(token_type::SQBRACKET_OPEN)))
    {
        arrayCount++;
        std::pair<bool, bool> flag;
        
        if(!follows(token_type::SQBRACKET_CLOSED))
            COMP_ERROR_R(RS_SYNTAX_ERROR, "Unclosed type specified array.", tinfo);

        if (follows(token_type::SYMBOL, '?'))
            flag.first = true;
        if (follows(token_type::SYMBOL, '&'))
            flag.second = true;
        if (flag.first && flag.second)
            COMP_ERROR(RS_SYNTAX_ERROR, "A type or type dimension cannot be both optional and a reference.");

        arrayFlags.push_back(flag);
    }
    if(tinfo.type_id == -1)
    {
        tinfo.type_id     = typeID;
        tinfo.reference   = reference;
        tinfo.optional    = optional;
        tinfo.array_count = arrayCount;
        tinfo.generic     = genericID != -1;
        tinfo.generic_id  = genericID;
        tinfo.arrayFlags  = arrayFlags;
    }
    else if (convertedFromGeneric)
    {
        // this is explicit functionality todo should revamp to use .assignType
        tinfo.array_count = arrayCount;
        tinfo.arrayFlags.insert(tinfo.arrayFlags.begin(), arrayFlags.begin(), arrayFlags.end());

        tinfo.optional = optional;
        tinfo.reference = reference;
        tinfo.generic = false; 
    } else
        tinfo.otherTypes.push_back(rs_type_info{typeID, arrayCount, optional, reference, genericID != -1, genericID, {}, arrayFlags});
    adv();

    nexpect('?', '|', '&');

    return tinfo;
};

// called at index of first generic
std::shared_ptr<rbc_function_generics> rbc_parser::genericsparse ()
{
    std::shared_ptr<rbc_function_generics> generics = std::make_shared<rbc_function_generics>();
    int info = 0;
    do
    {
        adv();

        bool isObjOnly = false;
        if (currentToken->type == token_type::TYPE_DEF && currentToken->info == RS_OBJECT_KW_ID)
        {
            isObjOnly = true;
            adv();
        }

        expect(token_type::WORD);

        std::string genericName = currentToken->repr;
        if (program.currentGenericTypes.find(genericName) != program.currentGenericTypes.end())
            COMP_ERROR(RS_SYNTAX_ERROR, "Expected generic type name.");

        generics->entries.insert({genericName, isObjOnly});
        adv();
        info = currentToken->info;

        expect_any(',', '>');
    } while (info != '>');
    
    return generics;
}
void                          rbc_parser::useNextFile            ()
{
    if (files.empty()) return;
    currentFile = files.front();
    program.currentFragment = currentFile;
    files.pop_front();

    tokens = &currentFile->tokens;
    S = tokens->size();
    content = &currentFile->fileContent;
    
    _At = 0;
    resync();
}

std::shared_ptr<rbc_function> rbc_parser::instantiateGenericFunction (const std::vector<rs_type_info>& types, std::shared_ptr<rbc_function> function)
{
    std::shared_ptr<rbc_function> copiedFunc = std::make_shared<rbc_function>(*function, types);
    copiedFunc->assignedGenerics = std::make_shared<std::vector<rs_type_info>>(types);
    const size_t S = copiedFunc->generics->entries.size();
    const size_t P = types.size();

    if (P != S)
        COMP_ERROR(RS_SYNTAX_ERROR, "Expected {} generic arguments, {} provided.", S, P);

    // dont need to worry about parent functions, generic functions cannot have parent functions

    program.currentFunction        = copiedFunc;
    program.genericTypeConversions = types;
    
    
    for(auto& entry : copiedFunc->generics->entries)
        program.currentGenericTypes.insert({ entry.first, {program.currentGenericTypes.size(), entry.second} });

    size_t beforeAt = _At;
    size_t oldScope = program.currentScope;

    // [] <- C, 2, 4
    // b = C
    // [] <- G, C, 2, 4
    files.push_front(copiedFunc->generics->fromFragment);

    useNextFile();

    program.currentScope = copiedFunc->scope + 1;

    _At = copiedFunc->generics->begin;
    size_t end = copiedFunc->generics->end;

    while (adv() && _At < end)
        parseCurrent();

    // parse the function copiedFunc

    useNextFile();

    _At                  = beforeAt;
    program.currentScope = oldScope; 

    program.currentFunction = nullptr;
    program.genericTypeConversions.clear();
    program.currentGenericTypes.clear();
    return copiedFunc;
}
bool                         rbc_parser::callparse        (std::string& name,
                                                           bool needsTermination,
                                                           std::shared_ptr<rs_module> fromModule,
                                                           rs_type_info* expectedReturnType)
{
    std::unordered_map<std::string, std::shared_ptr<rbc_function>>::iterator func;
    std::shared_ptr<rbc_function> function = nullptr;
    std::vector<rs_type_info> generics;

    bool _providedGenericTypes = false;
    bool internal = false;
    bool _extern  = false;

    if (fromModule)
        func = fromModule->functions.find(name);
    else
        func = program.functions.find(name);


    if (func == program.functions.end())
    {
        if (!program.currentFunction)
        {
        _notfound:
            COMP_ERROR_R(RS_SYNTAX_ERROR, "Unknown function name.", false);
        }
        
        auto child = program.currentFunction->childFunctions.find(name);

        if (child == program.currentFunction->childFunctions.end())
            goto _notfound;

        function = child->second;

        if (program.currentFunction->name == name)
            COMP_ERROR_R(RS_SYNTAX_ERROR, "Recursion is not supported yet.", false);

    } else function = func->second;

    auto& decorators = function->decorators;
    if (std::find(decorators.begin(), decorators.end(), rbc_function_decorator::CPP) != decorators.end())
        internal = true;
    if (std::find(decorators.begin(), decorators.end(), rbc_function_decorator::EXTERN) != decorators.end())
        _extern = true;

    if (equals(token_type::SYMBOL, '<'))
    {
        _providedGenericTypes = true;
        generics = generics_insersion_parse();
        adv();
        expect(token_type::BRACKET_OPEN);
    }

    // -- TODO -- REMOVE 
    const bool _functionHasGenerics = (bool)function->generics;
    size_t expectedGenericTypeCount = _functionHasGenerics ? function->generics->entries.size() : 0;

    if (_providedGenericTypes && _functionHasGenerics && !internal && generics.size() != expectedGenericTypeCount)
        COMP_ERROR(RS_SYNTAX_ERROR, "Expected passing of (all) generic types to generic function.");

    auto parseGenerics = [&]()
    {
        if(!_functionHasGenerics)
            COMP_ERROR(RS_SYNTAX_ERROR, "A function with no generics cannot be called with template arguments.");
        if (expectedReturnType)
        {
            rs_type_info returnType = *function->returnType;
            
            if (returnType.generic)
                returnType.assignType(generics.at(returnType.generic_id));

            if (!returnType.equals(*expectedReturnType))
                COMP_ERROR(RS_SYNTAX_ERROR, "Return type of instantiated generic function does not match the type the expression was expecting.");
        }

        auto& variations = function->generics->variations;
        std::unordered_map<std::vector<rs_type_info>, std::shared_ptr<rbc_function>>::iterator
            iter; 
        if ((iter = variations.find(generics)) != variations.end())
            function = iter->second;
        else if (!internal && !_extern) // we assume extern | internal functions handle the generics fine
        {
            std::shared_ptr<rbc_function> compiledFunc = instantiateGenericFunction(generics, function);
            
            function->generics->variations.insert({generics, compiledFunc});
            function = compiledFunc;
        }
    };
    if (_functionHasGenerics)
    {
        if (_providedGenericTypes)
            parseGenerics();
        else
            generics.resize(expectedGenericTypeCount);
    }

    if (function->parent && program.currentScope + 1 != function->scope)
        COMP_ERROR_R(RS_SYNTAX_ERROR, "Nested function definitions cannot be called outside their parent function body.", false);
    size_t pc = 0; // param count
    token* start = currentToken;
    
    adv();

    // -- TODO -- IMPL
    // generics will now always exist, we try to infer all the types needed in the parameter parsing below
    // if (function->generics && !generics)
    // {
    //     generics = new std::vector<rs_type_info>();
    //     generics->reserve(function->generics->entries.size());
    // }
    // std::unordered_map<std::string, bool>& genericEntries = function->generics->entries;

    if (currentToken->type != token_type::BRACKET_CLOSED)
    {
        while(1)
        {
            rs_expression expr = expreval(true, false, false);
            resync(); // reassign current
            if (expr.nonOperationalResult)
                adv(); // object parsing finishes at }, not: ,
            auto result = expr.rbc_evaluate(program);

            rs_variable* param = function->getNthParameter(pc);
            if (!param)
                COMP_ERROR_R(RS_SYNTAX_ERROR, "No matching function call with pc of {}", false, pc);

            rs_type_info tinfo = param->type_info;

            if (tinfo.generic)
            {
                // TODO: possibly could merge these ifs.
                if (_providedGenericTypes)
                {
                    // explicit functionality
                    rs_type_info& passedType = generics.at(tinfo.generic_id);

                    tinfo.assignType(passedType);

                    if (!internal && !_extern)
                        param->type_info.assignType(tinfo);

                    typeverify(tinfo, result, RS_PARSER_PARAMETER_USE_CASE);
                    
                }
                else
                {
                    rs_type_info& finalType = generics.at(tinfo.generic_id);

                    bool typeExists = finalType.type_id != -1;
                    if (typeExists)
                        tinfo.assignType(finalType, true); // if the type has been defined, it will be explicitly set everywhere else.
                    else
                        tinfo.assignType(typeinfer(result), false);
                    
                    if (!typeExists)
                        finalType.assignType(tinfo, false); // implicitly assign to T

                    typeverify(tinfo, result, RS_PARSER_PARAMETER_USE_CASE);
                }
            }
            else
                typeverify(tinfo, result, RS_PARSER_PARAMETER_USE_CASE);


            rbc_command c(rbc_instruction::PUSH,
                    rbc_constant(token_type::STRING_LITERAL, function->name),
                    rbc_constant(token_type::STRING_LITERAL, param->name), result);

            if (fromModule)
                c.parameters.push_back(std::make_shared<rbc_value>(fromModule));
            program(c);
            pc ++;
            if (equals(token_type::SYMBOL, ','))
                adv();
            else if (equals(token_type::BRACKET_CLOSED))
                break;
            else
                COMP_ERROR_R(RS_SYNTAX_ERROR, "Unexpected token.", false);
        }
    }
    
    // valiate we have inferred all generics
    if (_functionHasGenerics && !_providedGenericTypes)
    {
        for (const auto& t : generics)
        {
            if (t.type_id == -1) // is null
                COMP_ERROR(RS_SYNTAX_ERROR, "Could not infer all generic types through passed parameter types. Try calling the function by explicitly defining all generic types.");
        }
        _providedGenericTypes = true;

        parseGenerics();
        
    }

    if(_At >= S)
    {
        // for error checking
        currentToken = start;
        COMP_ERROR_R(RS_SYNTAX_ERROR, "Missing closing bracket | semi-colon.", false);
    }
    size_t actualpc = function->parameters.size();
    if (actualpc != pc)
        COMP_ERROR_R(RS_SYNTAX_ERROR, "No matching function call with pc of {}", false, pc);
    rbc_command c(rbc_instruction::CALL);

    // a function with a parent or a function with generics cannot be found by the 
    // transpiler without info, so we directly pass the pointer of the function to the asm->mcfunction compiler.
    if (!function->parent && !function->generics)
        c.parameters.push_back(std::make_shared<rbc_value>(rbc_constant(token_type::STRING_LITERAL, name, std::make_shared<raw_trace_info>(start->trace))));
    else
    {
        // pass mem addr of function to instruction as its a child function or generic function
        // and impossible to find otherwise
        // as well as generics for helpfullness...
        c.parameters.push_back(std::make_shared<rbc_value>(std::static_pointer_cast<void>(function)));
        auto ptr = std::make_shared<std::vector<rs_type_info>>(generics);
        c.parameters.push_back(std::make_shared<rbc_value>(ptr));
    }
    if (fromModule)
        c.parameters.push_back(std::make_shared<rbc_value>(rbc_value(fromModule)));
    
    program(c);
    // below is handled with stack frames now.
    // if (!internal)
    //     for(size_t i = 0; i < pc; i++)
    //         program(rbc_command(rbc_instruction::POP));
    
    if (needsTermination)
    {
        adv();
        expect(token_type::LINE_END);
    }
    return true;
}
std::shared_ptr<rs_variable> rbc_parser::varparse         (token& name, bool needsTermination, bool parameter, bool obj, bool isConst)
{
    if (program.functions.find(name.repr) != program.functions.end()
    || (program.currentFunction && program.currentFunction->name == name.repr))
        COMP_ERROR_R(RS_SYNTAX_ERROR, "The name '{}' already exists as a function.", nullptr, name.repr);
    std::shared_ptr<rs_variable> variable = program.getVariable(name.repr);
    // if we access the variable like v[0].b = ...
    std::shared_ptr<rs_var_access_path> accessingPath = nullptr;
    bool exists = (bool)variable; // && variable->scope == program.currentScope
    
// _eval:
    if (currentToken->info != ':')
    {
        if (!exists)
            COMP_ERROR_R(RS_SYNTAX_ERROR, "Expected type.", nullptr);
            
        if (equals(token_type::SQBRACKET_OPEN) || equals(token_type::OBJECT_ACCESS_OPERATOR))
        {
            if (parameter || obj)
                COMP_ERROR(RS_SYNTAX_ERROR, "Accessing a list/object variable is not allowed here.");
            
            accessingPath = std::make_shared<rs_var_access_path>(parse_var_path_access(variable));
            if (equals(token_type::LINE_END))
                return variable;
        }
        goto _skip_type;
    }
    else if (exists)
    {
        COMP_ERROR_R(RS_SYNTAX_ERROR, "Variable cannot be reinitialized again.", nullptr);
    }
    else
    {
        rs_type_info type = typeparse();

        variable = std::make_shared<rs_variable>(name, program.currentScope, !program.currentFunction);
        variable->type_info = type;
        variable->_const = isConst;
    }


_skip_type:
    rs_type_info& typeInfo = (accessingPath ? accessingPath->evaluatedType : variable->type_info);

    switch(currentToken->info)
    {
    case '=': // assign a value to variable
    {
        bool needsCreation = !exists;
        if(!variable)
        {
            variable = std::make_shared<rs_variable>(name, program.currentScope, !program.currentFunction);
            variable->_const = isConst;
            needsCreation = true;
        }

        if (exists && variable->_const)
            COMP_ERROR_R(RS_SYNTAX_ERROR, "Cannot reassign constant variable.", nullptr);

        adv();
        token* next = nullptr;
        token_type currentType = currentToken->type;

        // the functionality below really should just be handled in parsemoduleusage().
        if (currentType == token_type::WORD && (next = peek()) && next->type == token_type::MODULE_ACCESS)
        {
        _parseModule:
            auto _module = program.modules.find(currentToken->repr);

            if (_module == program.modules.end())
                COMP_ERROR(RS_SYNTAX_ERROR, "Unknown module name.");
            
            adv();

            if (!parsemoduleusage(_module->second, &typeInfo))
                ABORT_PARSE;

            if (accessingPath)
                program(rbc_commands::variables::storeReturn(accessingPath));
            else
                
                program(rbc_commands::variables::storeReturn(variable));
            break;
        }
        else if (currentType == token_type::MODULE_ACCESS)
        {
            if (program.currentModule)
            {
                if (!parsemoduleusage(program.currentModule, &typeInfo))
                    ABORT_PARSE;
            }
            else
            {
                adv();
                expect(token_type::WORD);
                if ((next = peek()) && next->type == token_type::MODULE_ACCESS)
                    goto _parseModule;
                goto _parseFunc;
            }

            break;
        }
        else if (currentType == token_type::WORD && (next = peek())
                                      && (next->type == token_type::BRACKET_OPEN ||
                                            (next->type == token_type::SYMBOL && next->info == '<')))
        {
        _parseFunc:
        
            // its a function call, function calls are expensive and only allowed once in an expression,
            // hence why we skip expreval here.
            std::string& funcname = currentToken->repr;
            auto f = program.functions.find(funcname);
            if (f == program.functions.end())
                COMP_ERROR(RS_SYNTAX_ERROR, "Unknown function name.");
            
            auto& decorators = f->second->decorators;
            if(std::find(decorators.begin(), decorators.end(), rbc_function_decorator::NORETURN) != decorators.end())
                COMP_ERROR_R(RS_SYNTAX_ERROR, "Cannot assign variable the value of a function that is marked as 'noreturn'.", nullptr);

            if (!f->second->returnType->equals(typeInfo))
                COMP_ERROR_R(RS_SYNTAX_ERROR, "Function return type does not match or convert to variable type.", nullptr);

            adv();

            if (!callparse(funcname, false, nullptr, &typeInfo))
                return nullptr;
            adv();

            expect(token_type::LINE_END, "Missing semi-colon. This error can arise if you are calling a function within an expression. Function calls are not allowed in arithmetic expressions.");
            if (needsCreation)
            {
                program(rbc_commands::variables::create(variable));
            }
            if (accessingPath)
                program(rbc_commands::variables::storeReturn(accessingPath));
            else
                program(rbc_commands::variables::storeReturn(variable));
            
            break;
        }

        using _ValueT = bst_operation<token>::_NodeT;

        rs_expression expr = expreval();
        variable->value = std::make_shared<rs_expression>(expr);
        
        if (expr.operation.isSingular() && !obj && !expr.nonOperationalResult)
        {
            _ValueT& leftVal = std::get<_ValueT>(*expr.operation.left);

            if (leftVal.index())
            {
                // convert path to rbc value and let transpiler convert it
                rbc_value path = std::get<rs_var_access_path>(leftVal);

                if (needsCreation)
                    program(rbc_commands::variables::create(variable, path));
                else
                {
                    if (accessingPath)
                        program(rbc_commands::variables::set(accessingPath, path));
                    else
                        program(rbc_commands::variables::set(variable, path));

                }
            }
            else
            {
                token& value = std::get<token>(leftVal);
                if (value.type == token_type::WORD)
                {
                    sharedt<rs_variable> var = program.getVariable(value.repr);

                    if (!typeInfo.equals(var->type_info))
                        COMP_ERROR(RS_SYNTAX_ERROR, "Cannot copy varaible of to variable of different type.");
                }
                else if (!typeInfo.equals(value.info))
                    COMP_ERROR_R(RS_SYNTAX_ERROR, "Cannot assign constant of type {} to variable of type {}.", nullptr, tutil::type_to_str(value.info), typeInfo.full_type_name());
                
                rbc_value val = value.type == token_type::WORD ? program.getVariable(value.repr) : rbc_value(rbc_constant(value.type, value.repr, std::make_shared<raw_trace_info>(value.trace)));
                // no need to evaluate.
                if (needsCreation)
                    program(rbc_commands::variables::create(variable, val));
                else
                {
                    if (accessingPath)
                        program(rbc_commands::variables::set(accessingPath, val));
                    else
                        program(rbc_commands::variables::set(variable, val));

                }
            }
            
        }
        else if (!obj)
        {
            rbc_value result = expr.rbc_evaluate(program);

            // list
            if (result.index() == 4)
            {
                if (typeInfo.array_count == 0)
                    COMP_ERROR(RS_SYNTAX_ERROR, "Cannot assign list-instance to a non-list typed variable.");
            }

            typeverify(typeInfo, result, RS_PARSER_VARIABLE_USE_CASE);

            if (needsCreation)
                program(rbc_commands::variables::create(variable, result));
            else
            {
                if (accessingPath)
                    program(rbc_commands::variables::set(accessingPath, result));
                else
                    program(rbc_commands::variables::set(variable, result));

            }
            // singular expressions need to adv
            // adv();
        }
        break;
    }
    case ';':
    {
        if (!exists)
            program(rbc_commands::variables::create(variable));
        break;
    }
    case ',':
    case ')':
    case ']':
        if (parameter) break;
        __attribute__((fallthrough));
    default:
        WARN("Unexpected token after variable usage ('%s').", currentToken->repr.c_str());
        break;
    }

    resync();

    if (needsTermination)
        expect(token_type::LINE_END);

    if (!exists && variable && !obj)
    {
        if (isConst)
            variable->_const = true;
        if(!program.currentFunction)
            program.globalVariables.push_back(variable);
        else
        {
            if (parameter)
            {
                variable->comp_info.isParameter = true;
                program.currentFunction->parameters.push_back(variable);
            }
            program.currentFunction->localVariables.insert({variable->name, rbc_func_var_t{variable, parameter}});
        }
    }
    return variable;
}
bool                         rbc_parser::parsemoduleusage (std::shared_ptr<rs_module> currentModule, rs_type_info* expectedReturnType)
{
    do
    {
        adv();
        auto module_iter = currentModule->children.find(currentToken->repr);
        if (module_iter == currentModule->children.end())
            break; // could be invalid name, or function name.
        currentModule = module_iter->second;
        
        adv();
    }
    while(currentToken->type == token_type::MODULE_ACCESS);

    std::string funcName = currentToken->repr;
    
    adv();
    if(!callparse(funcName, true, currentModule, expectedReturnType))
        return false;

    return true;
}
// called at index of <
std::vector<rs_type_info>    rbc_parser::generics_insersion_parse ()
{
    std::vector<rs_type_info> ret;

    while(currentToken->info != '>')
    {
        rs_type_info info = typeparse();

        ret.push_back(info);

        expect_any(',', '>');
    }

    return ret;
}
std::shared_ptr<rs_object>   rbc_parser::objparse         (std::string& name)
{
    expect(token_type::CBRACKET_OPEN);


    rs_object obj{name, program.currentScope};
    rs_object_member_decorator decorator = rs_object_member_decorator::OPTIONAL;
    while(adv() && currentToken->type != token_type::CBRACKET_CLOSED)
    {
        // dont append to global scope
        token* name;
        token_type& t = currentToken->type;
        switch(t)
        {
            case token_type::WORD:
                name = currentToken;
                break;
            case token_type::KW_OPTIONAL:
            case token_type::KW_REQUIRED:
            case token_type::KW_SEPERATE:
            {
                if (t == token_type::KW_REQUIRED)
                    decorator = rs_object_member_decorator::REQUIRED;
                else if (t == token_type::KW_SEPERATE)
                    decorator = rs_object_member_decorator::SEPERATE;

                adv();
                expect(token_type::WORD);
                
                name = currentToken;
                break;
            }
            default:
                COMP_ERROR_R(RS_SYNTAX_ERROR, "Unexpected token.", nullptr);
        }
        adv();
        auto variable = varparse(*name, true, false, true);
        if (obj.members.find(variable->name) != obj.members.end())
            COMP_ERROR_R(RS_SYNTAX_ERROR, "Duplicate object member name.", nullptr);

        obj.members.insert({variable->name, {*variable, decorator}});
    }
    if (_At == S - 1)
        COMP_ERROR_R(RS_EOF_ERROR, "Unterminated object body.", nullptr);
    return std::make_shared<rs_object>(obj);
}
rs_type_info                 rbc_parser::typeinfer        (const rbc_value& val)
{
    switch(val.index())
    {
        case 0:
            return rs_type_info{static_cast<int>(std::get<0>(val).val_type)};
        case 1:
        {
            auto& reg = std::get<1>(val);
            if (reg->operable)
                return rs_type_info{RS_INT_KW_ID};
            return rs_type_info{}; // unsupported.
        }
        case 2:
        {
            auto& var = std::get<2>(val);

            return var->type_info;
        }
        case 4:
        {
            auto& list = std::get<4>(val);

            return list->elementType.parent_type();
        }
        case 6:
        {
            auto& path = std::get<6>(val);

            return path.evaluatedType;
        }
        default:
            return rs_type_info{};
    }
}
void                         rbc_parser::parseCurrent     ()
{
    switch(currentToken->type)
    {
    case token_type::WORD:
    {
    // _parseword:
        token& word = *currentToken;
        
        if (follows(token_type::BRACKET_OPEN) || follows(token_type::SYMBOL, '<'))
        {
            if (!callparse(word.repr, true, nullptr))
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
        else if (follows(token_type::SYMBOL) || follows(token_type::SQBRACKET_OPEN) || follows(token_type::OBJECT_ACCESS_OPERATOR))
        {
            // variable usage.
            if(program.currentModule)
                COMP_ERROR(RS_SYNTAX_ERROR, "Variable usage not allowed in modules.");
            token* before = peek(-2);
            if (!varparse(word, 
                true,
                false,
                false,
                before && before->type == token_type::KW_CONST))
                ABORT_PARSE;
        }
        else COMP_ERROR(RS_SYNTAX_ERROR, "Unexpected token.");
        break;
    }
    case token_type::MODULE_ACCESS:
    {
        // using currentToken module access operator (::dostuff)
        if (program.currentModule)
        {
            if(!parsemoduleusage(program.currentModule))
                ABORT_PARSE;
        }
        else if (!follows(token_type::WORD))
            COMP_ERROR(RS_SYNTAX_ERROR, "Expected module name.");
        break;
    }
    case token_type::KW_MODULE:
    {
        adv();
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
        
        program.currentScope++;

        adv();
        expect(token_type::CBRACKET_OPEN);
        // don't change scope! - Future K: WHY?
        break;
    }
    case token_type::KW_METHOD:
    {
#pragma region function_definitions
        adv();
        
        rs_type_info retType;
        std::string name;
        std::shared_ptr<rbc_function_generics> generics = nullptr;
    _type_parse:
        switch (currentToken->info)
        {
            case ':':
                break;
            case '<':
            {
                if (generics)
                    COMP_ERROR(RS_SYNTAX_ERROR, "Unexpected token.");

                if (program.functionStack.size() > 0)
                    COMP_ERROR(RS_SYNTAX_ERROR, "Nested functions cannot be generic.");
                // parse function generics
                generics = genericsparse();

                // merge
                for (auto& genericType : generics->entries)
                    program.currentGenericTypes.insert({genericType.first, { program.currentGenericTypes.size(), genericType.second }});

                generics->fromFragment = currentFile;

                adv();
                goto _type_parse;
            }
            default:
                COMP_ERROR(RS_SYNTAX_ERROR, "Missing function return type.");

        }
        // we are defining the return type
        retType = typeparse();
        if(!retType.generic && retType.equals(RS_NULL_KW_ID))
            COMP_ERROR(RS_SYNTAX_ERROR, "A functions' return type cannot be marked as null, use 'void' instead.");            
        
        expect(token_type::WORD);

        name = currentToken->repr;
        
        if (program.currentModule)
        {
            auto& functions = program.currentModule->functions;
            if (functions.find(name) != functions.end())
                COMP_ERROR(RS_SYNTAX_ERROR, "Function already exists in module.");
        }
        else if (program.functions.find(name) != program.functions.end())
            COMP_ERROR(RS_SYNTAX_ERROR, "Function already exists.");

        adv();
        
        expect(token_type::BRACKET_OPEN);
        
        for(char c : name)
        {
            if (std::isupper(c))
                COMP_ERROR(RS_SYNTAX_ERROR, "Function names cannot have uppercase letters due to how minecraft functions are implemented. Only use lower case letters and underscores.");
        }
        if (program.currentFunction)
            program.functionStack.push(program.currentFunction);

        program.currentFunction = std::make_shared<rbc_function>(name);
        program.currentFunction->generics   = generics;
        program.currentFunction->scope      = program.currentScope;
        program.currentFunction->returnType = std::make_shared<rs_type_info>(retType);
        program.scopeStack.push(rbc_scope_type::FUNCTION);

        if (program.currentModule)
            program.currentFunction->modulePath = program.currentModule->modulePath;

        program.currentScope ++;

        do
        {
            adv();
            bool _const = false;
            switch(currentToken->type)
            {
                case token_type::BRACKET_CLOSED:
                    goto after_param;
                case token_type::WORD:
                {
                _parseVar:
                    token& varName = *currentToken;
                    adv();

                    // false as we do not need to terminate variable usage with ; or =
                    if (!varparse(varName, false, true, false, _const)) // varparse adds any instructions to program.currentFunction
                        ABORT_PARSE;
                    if (currentToken->type == token_type::BRACKET_CLOSED)
                        goto after_param;
                    break;
                }
                case token_type::KW_CONST:
                {
                    _const = true;
                    adv();
                    goto _parseVar;
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
                program._debug_infuncbody = true;
                // break, hand over parsing to main loop, scope is incremented earlier for parameters. it will decrement when it reaches
                // a closing curly brace.
                if (generics)
                {
                    // because we have generics, we need to parse the function every time we call it with a different type.
                    // so we skip over the body, and parse it (if it exists) when we get the type info we need.
                    token* at = currentToken;
                    int bc = 1;
                    generics->begin = _At;
                    while (bc > 0 && adv())
                    {
                        if (equals(token_type::CBRACKET_OPEN))
                            bc++;
                        if (equals(token_type::CBRACKET_CLOSED))
                            bc--;
                    }
                    if (!resync())
                    {
                        currentToken = at;
                        COMP_ERROR(RS_SYNTAX_ERROR, "Unmatched closing bracket.");
                    }
                    generics->end = _At;
                    goto _decrement_scope;
                }
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
                program._debug_infuncbody = false;
                if (program.currentFunction->generics)
                {
                    for(auto& generic : program.currentFunction->generics->entries)
                        program.currentGenericTypes.erase(generic.first);
                }
                 
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

        adv();
        
        if (currentToken->type == token_type::LINE_END)
        {
            if (!program.currentFunction->returnType->equals(RS_VOID_KW_ID))
                COMP_ERROR(RS_SYNTAX_ERROR, "Cannot return nothing to a function with a return type of non-void.");
            program(rbc_command(rbc_instruction::RET));
            break;
        }

        // possible expression OR function call.
        token& start = *currentToken;

        
        if(follows(token_type::BRACKET_OPEN) || follows(token_type::SYMBOL, '<'))
        {
        _parseFunc:
            if(!callparse(start.repr, true, nullptr))
                ABORT_PARSE;
        }
        else if (equals(token_type::MODULE_ACCESS))
        {
            if (program.currentModule)
            {
                if(!parsemoduleusage(program.currentModule))
                    ABORT_PARSE;
            }
            else
            {
                adv();
                expect(token_type::WORD);
                token* next;
                if ((next = peek()) && next->type == token_type::MODULE_ACCESS)
                    goto _parseModule;

                goto _parseFunc;
            }
        }
        else if (follows(token_type::MODULE_ACCESS))
        {
        _parseModule:
            auto _module = program.modules.find(currentToken->repr);
            if (_module == program.modules.end())
                COMP_ERROR(RS_SYNTAX_ERROR, "Unknown module name.");

            if (!parsemoduleusage(_module->second))
                ABORT_PARSE;
        }
        else
        {
            rs_expression expr = expreval();
            rbc_value result = expr.rbc_evaluate(program); // evaluate and compute return statement
            
            typeverify(*program.currentFunction->returnType, result, RS_PARSER_RETURN_USE_CASE);

            program(rbc_command(rbc_instruction::RET, result));
        }

        break;
    }
    case token_type::KW_IF:
    {
    _parseif:
        adv();
        expect(token_type::BRACKET_OPEN);
            
    _parseifagain: // for and and or keywords
        adv();
        // br = true as if we hit the closing bracket of the if statement we should return.
        rs_expression left = expreval(true, false, false);
        rbc_value lVal = left.rbc_evaluate(program);
        
        resync();

        if (currentToken->type == token_type::BRACKET_CLOSED)
        {
            program(rbc_command(flags.parsingelif ? rbc_instruction::ELIF : rbc_instruction::IF, lVal));
        end_if_parse:
            adv();

            expect(token_type::CBRACKET_OPEN);
            
            program.currentScope++;
            program.scopeStack.push(flags.parsingelif ? rbc_scope_type::ELIF : rbc_scope_type::IF);
            break;
        }

        token& op         = *currentToken;
        token_type compop = op.type;   
        if(compop != token_type::COMPARE_EQUAL && compop != token_type::COMPARE_NOTEQUAL)
            COMP_ERROR(RS_SYNTAX_ERROR, "Unexpected token.");

        adv();
        // br = true as if we hit the closing bracket of the if statement we should return.
        // prune = false as expreval has mismatching problems after when encountering this ).
        // horrible code having 4 boolean flags, maybe make struct.
        rs_expression right = expreval(true, false, false);
        resync();
        rbc_value rVal = right.rbc_evaluate(program);
        token_type t = currentToken->type;

        switch(t)
        {
            case token_type::KW_AND:
            case token_type::KW_OR:
                adv();
                goto _parseifagain;
            case token_type::BRACKET_CLOSED:
                break;
            default:
                COMP_ERROR(RS_SYNTAX_ERROR, "Unexpected token.");

        }
        program(rbc_command(flags.parsingelif ? rbc_instruction::ELIF : rbc_instruction::IF, lVal, rbc_constant(compop, op.repr, std::make_shared<raw_trace_info>(op.trace)), rVal));
        
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
        
        adv();
        if (currentToken->type != token_type::CBRACKET_OPEN)
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
                adv();
                
                std::string& name = currentToken->repr;

                
                expect(token_type::WORD);

                if (program.objectTypes.find(name) != program.objectTypes.end())
                    COMP_ERROR(RS_SYNTAX_ERROR, "Object with name already exists.");
                
                adv();

                auto obj = objparse(name);
                obj->typeID = rs_object::TYPE_CARET_START + program.objectTypes.size();
                program.objectTypes.insert({name, obj});
                break;
            }
            default:
                // WARN("Unimplemented type.");
                break;
        }
        
        break;
    }
    default:
    {
        // std::string str = currentToken->str();
        // WARN("Found suspicious token (possible due to a non implemented feature): %s.", str.c_str());
        break;
    }
    }
}
