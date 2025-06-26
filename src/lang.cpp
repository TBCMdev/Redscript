#include "lang.hpp"
#include "rbc.hpp"
#define COMP_ERROR(_ec, _message, _trace, ...)                                                              \
    {                                                                                                       \
        err = rs_error(_message, *program.context->content, _trace, program.context->fName, ##__VA_ARGS__); \
        err.trace.ec = _ec;                                                                                 \
        return;                                                                                             \
    }
#define EXPR_ERROR(_ec, _message, _trace, ...)                                                               \
    {                                                                                                        \
        *err = rs_error(_message, *program.context->content, _trace, program.context->fName, ##__VA_ARGS__); \
        err->trace.ec = _ec;                                                                                 \
        return root;                                                                                         \
    }
#define EXPR_ERROR_R(_ec, _message, _trace, _ret, ...)                                                       \
    {                                                                                                        \
        *err = rs_error(_message, *program.context->content, _trace, program.context->fName, ##__VA_ARGS__); \
        err->trace.ec = _ec;                                                                                 \
        return _ret;                                                                                         \
    }
#pragma region objects

std::shared_ptr<rs_object> parseInlineObject(rbc_program& program, token_list& tlist, size_t& start, rs_error* err)
{
    size_t S = tlist.size();
    rs_object obj;
    while(start + 1 < S)
    {
        token& name = tlist.at(++start);
        if (name.type == token_type::CBRACKET_CLOSED)
            break;
        if (name.type != token_type::WORD)
            EXPR_ERROR_R(RS_SYNTAX_ERROR, "Unexpected token.", name.trace, nullptr);
        if (obj.members.find(name) != obj.members.end())
            EXPR_ERROR_R(RS_SYNTAX_ERROR, "Duplicate object field.", name.trace, nullptr);

        if (start + 1 >= S)
            break;
        token& sep = tlist.at(++start);
        if (sep.type != token_type::SYMBOL || sep.info != ':')
            EXPR_ERROR_R(RS_SYNTAX_ERROR, "Expected ':'.", sep.trace, nullptr);

        start++;

        rs_expression value = expreval(program, tlist, start, err, false, false, true);
        if (value.nonOperationalResult)
            start++;
        if (err->trace.ec)
            return nullptr;

        rs_variable var(name, program.currentScope);
        var.value = std::make_shared<rs_expression>(value);
        obj.members.insert({name.repr, {var, rs_object_member_decorator::OPTIONAL}});

        token& terminator = tlist.at(start);
        if (terminator.type == token_type::CBRACKET_CLOSED)
            break;
        if (terminator.type != token_type::SYMBOL || terminator.info != ',')
            EXPR_ERROR_R(RS_SYNTAX_ERROR, "Expected object field seperator or '}'.", terminator.trace, nullptr);
    }
    if (start + 1 >= S)
        EXPR_ERROR_R(RS_SYNTAX_ERROR, "Unterminated object definition.", tlist.back().trace, nullptr);

    return std::make_shared<rs_object>(obj);
    
}

#pragma endregion objects
#pragma region expressions

rs_expression::_ResultT rs_expression::rbc_evaluate(rbc_program& program, rs_error* err,
                                          bst_operation<token>* node)
{
    using _NodeT  = bst_operation<token>;
    using _ValueT = rbc_value;

    if (nonOperationalResult)
        return *nonOperationalResult;

    if (!node) node = &operation;

    const bool leftIsToken  = node->left->index();
    const bool rightIsToken = node->right && node->right->index();


    std::shared_ptr<_ValueT> leftVal;
    std::shared_ptr<_ValueT> rightVal;

    // an operable computation is made when the left and right parts of the node contain integer values.
    // whether that be an integer or a variable holding an integer.
    // a quick fix is to set this = true, letting all computations become operable (except for ones with non operable registers)
    bool operableRegister     = true; // was false, error here. TODO fix

    if (!leftIsToken)
    {
        auto lresult = rbc_evaluate(program, err, &std::get<_NodeT>(*node->left));
        if (err->trace.ec)
            return lresult;
        if (lresult.index())
            leftVal = std::make_shared<_ValueT>(std::get<1>(lresult));
        else
        {
            sharedt<rbc_register>& reg = std::get<sharedt<rbc_register>>(lresult);
            leftVal = std::make_shared<_ValueT>(reg);
            operableRegister = reg->operable;
        }
    }else 
    {
        token& value = std::get<token>(*node->left);
        std::shared_ptr<rs_variable> var;
        if ((var = program.getVariable(value)))
            leftVal = std::make_shared<_ValueT>(var);
        else
            leftVal = std::make_shared<_ValueT>(rbc_constant(value.type, value.repr, &value.trace));
    }

    if(!node->right)
        return *leftVal;

    if (!rightIsToken)
    {
        auto rresult = rbc_evaluate(program, err, &std::get<_NodeT>(*node->right));
        if (err->trace.ec)
            return *leftVal;
        if(rresult.index() != 1)
            rightVal = std::make_shared<_ValueT>(rresult);
        else
        {
            sharedt<rbc_register>& reg = std::get<sharedt<rbc_register>>(rresult);
            if (operableRegister && !reg->operable)
                EXPR_ERROR_R(RS_UNSUPPORTED_OPERATION_ERROR,
                    "Unsupported operation between operable and non operable register. If you see this particular message, flag an error on the github.",
                    stack_trace(), *leftVal);
            rightVal = std::make_shared<_ValueT>(reg);
        }

    }else 
    {
        token& value = std::get<token>(*node->right);
        std::shared_ptr<rs_variable> var;
        if ((var = program.getVariable(value)))
            rightVal = std::make_shared<_ValueT>(var);
        else
            rightVal = std::make_shared<_ValueT>(rbc_constant(value.type, value.repr, &value.trace));
    }
    sharedt<rbc_register> reg = nullptr;
    bool occupy = true;
    if (leftVal->index() == 1)
    {
        reg = std::get<1>(*leftVal);
        if (!reg->vacant) 
        {
            reg = program.getFreeRegister(operableRegister);
            if (!reg)
                reg = program.makeRegister(operableRegister);
        } else occupy = false; // use register from prev operation to store this operation
    }else
    {
        reg = program.getFreeRegister(operableRegister);
        if (!reg)
            reg = program.makeRegister(operableRegister);
    }
    if (occupy)
        program (rbc_commands::registers::occupy(reg, *leftVal));
    program (rbc_commands::registers::operate(reg, *rightVal, static_cast<uint>(node->operation)));
    reg->free();

    return reg;
}
bst_operation<token> make_bst(rbc_program &program, token_list &tlist, size_t &start, rs_error *err, bool br, bool oneNode, bool obj)
{
    const size_t S = tlist.size();
    bst_operation<token> root;
    do
    {
        token &current = tlist.at(start);
        switch (current.type)
        {
        case token_type::LINE_END:
            return root;
        case token_type::BRACKET_CLOSED:
        {
            if (!br)
                EXPR_ERROR(RS_SYNTAX_ERROR, "Unclosed bracket found.", current.trace);
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
            bst_operation<token> child = make_bst(program, tlist, ++start, err, true, false);
            if (err->trace.ec)
                return root;

            if (!root.assignNext(child))
                EXPR_ERROR(RS_SYNTAX_ERROR, "Missing operator.", tlist.at(start).trace);
            // start ++;
            if (oneNode)
                return root;
            break;
        }
        case token_type::OPERATOR:
        {
            if (root.operation != bst_operation_type::NONE)
                EXPR_ERROR(RS_SYNTAX_ERROR, "Unexpected token.", current.trace);
            if (current.repr.length() > 1)
                EXPR_ERROR(RS_SYNTAX_ERROR, "Unexpected operator.", current.trace);
            const char op = current.info;

            if (!root.setOperation(op))
                EXPR_ERROR(RS_SYNTAX_ERROR, "Unsupported operator.", current.trace);

            break;
        }
        case token_type::INT_LITERAL:
        case token_type::FLOAT_LITERAL:
        case token_type::STRING_LITERAL:
        case token_type::LIST_LITERAL:
        case token_type::OBJECT_LITERAL:
        case token_type::WORD:
        {

            if (current.type == token_type::WORD)
            {
                if (!program.getVariable(current.repr))
                    EXPR_ERROR(RS_SYNTAX_ERROR, "Unexpected token in expression.", current.trace);
                if (!root.assignNext(current))
                    EXPR_ERROR(RS_SYNTAX_ERROR, "Missing operator.", current.trace);
            }
            else if (!root.assignNext(current))
                EXPR_ERROR(RS_SYNTAX_ERROR, "Missing operator.", current.trace);

            if (oneNode || (start + 1 < S && tlist.at(start + 1).type == token_type::LINE_END))
                return root;
            break;
        }
        default:
            // exceptions: these are all handled externally
            if ((current.info == ',' || current.type == token_type::SQBRACKET_CLOSED) ||
                (current.info == '}' && obj)         ||
                (current.type == token_type::COMPARE_EQUAL || current.type == token_type::COMPARE_NOTEQUAL))
            {
                return root; // commas can end expressions
            }
            EXPR_ERROR(RS_SYNTAX_ERROR, "Unknown token in expression.", current.trace);
        }
        if (root.right)
        {
            token* next = nullptr;
            while (start + 1 < S && (next = &tlist.at(start + 1))->type == token_type::OPERATOR)
            {
                if (next->repr.length() != 1) // todo remove 
                    EXPR_ERROR(RS_SYNTAX_ERROR, "Unsupported operator.", current.trace);
                const int pLeft = operatorPrecedence(root.operation), pRight = operatorPrecedence(next->info);

                start += 2;
                if (start >= S)
                    EXPR_ERROR(RS_SYNTAX_ERROR, "Expected expression, not EOF.", next->trace);
                // check if we are in a bracket, and advance to next token
                bool isBracketNode = tlist.at(start).type == token_type::BRACKET_OPEN;
                if (isBracketNode) start++;

                if (pRight < pLeft)
                {
                    // parse the next node only, concat root.right and new right to make more favorable node
                    bst_operation<token> right = make_bst(program, tlist, start, err, isBracketNode, true);
                    if (err->trace.ec)
                        return root;
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

                    bst_operation<token> right = make_bst(program, tlist, start, err, isBracketNode, true);
                    if (err->trace.ec)
                        return root;
                    // assign right
                    newRoot.assignNext(right);

                    root = newRoot;
                }
                if(isBracketNode) start++;
            }
            break;
        }
        
        if (start < S && tlist.at(start).type == token_type::LINE_END)
            return root;
    } while (++start < S);

    if (start > S)
        EXPR_ERROR(RS_EOF_ERROR, "Expected expression, not EOF.", tlist.at(S - 1).trace);

    return root;
}

void prune_expr(rbc_program& program, bst_operation<token>& expr, rs_error* err)
{
    using _NodeT = bst_operation<token>;
    /*
    x = (4 + 2), +, 2
    lt = false, rt = true

        lchild = (4 + 2)
        recur
            x = 4, +, 2
            lt = true, rt = true
            left = 4
            right = 2
            result = 6

            lcopy = 4
            lcopy.repr = result
            lcopy = 6
            - makeSingular -
                x.left = lcopy
                x.right = nothing
                x.operation = nothing
                x = 6, null, null

        lchild = 6, null, null
        if isSingular
            x.left = lchild.left
            x = 6, +, 2
            

    */
    bool isLeftToken  = expr.left->index();
    
    bool isRightToken = expr.right ? expr.right->index() : false;

    
    if (isLeftToken && isRightToken)
    {
        // compute
        token& left = std::get<token>(*expr.left);
        token& right = std::get<token>(*expr.right);

        std::string result;
        if(left.type == right.type)
        {
            switch (left.type)
            {
                case token_type::INT_LITERAL:
                {
                    int r  = operator_compute(std::stoi(left.repr), expr.operation, std::stoi(right.repr));
                    result = std::to_string(r);
                    break;
                }
                default:
                    WARN("No supported operation of same type (T=%d)", static_cast<int>(left.type));
                    break;
            }
        }
        else
        {
            WARN("No supported operation of same type (T=%d)", static_cast<int>(left.type));
        }
        if(!result.empty())
        {
            token copy = left;
            copy.repr = result;
            expr.makeSingular(copy);
        }

    }
    else
    {
        if(!isLeftToken)
        {
            _NodeT& lchild = std::get<_NodeT>(*expr.left);
            prune_expr(program, lchild, err);
            if (err->trace.ec)
                return;
            if (lchild.isSingular())
                expr.left = std::make_shared<_NodeT::_ValueT>(std::get<token>(*lchild.left));
        }
        if(!isRightToken && expr.right)
        {
            _NodeT& rchild = std::get<_NodeT>(*expr.right);
            prune_expr(program, rchild, err);
            if (err->trace.ec)
                return;
            if (rchild.isSingular())
                expr.right = std::make_shared<_NodeT::_ValueT>(std::get<token>(*rchild.left));
        }
        if (expr.left->index() && expr.right && expr.right->index())
            prune_expr(program, expr, err);
    }
    
}
// call after first [
std::shared_ptr<rs_list> parseList(rbc_program& program, token_list& tlist, size_t& start, rs_error* err)
{
    rs_list list;
    const size_t S = tlist.size();
    // just for error
    token& at = tlist.at(start);

    do
    {
        rs_expression expr = expreval(program, tlist, start, err, false, false);
        if (err->trace.ec)
            return nullptr;
        rs_expression::_ResultT val = expr.rbc_evaluate(program, err);
        if (err->trace.ec)
            return nullptr;

        list.values.push_back(std::make_shared<rbc_value>(val));
        at = tlist.at(start++);
    } while (start < S && at.type == token_type::SYMBOL && at.info == ',');

    if (at.type != token_type::SQBRACKET_CLOSED)
        EXPR_ERROR_R(RS_SYNTAX_ERROR, "Unclosed square bracket.", at.trace, nullptr);

    return std::make_shared<rs_list>(list);
}
rs_expression expreval(rbc_program &program, token_list &tlist, size_t& start, rs_error *err, bool br, bool lineEnd, bool obj, bool prune)
{
    rs_expression expr;
    token& current = tlist.at(start);
    if (current.type == token_type::CBRACKET_OPEN)
    {
        // parse object
        std::shared_ptr<rs_object> obj = parseInlineObject(program, tlist, start, err);
        if(err->trace.ec || !obj)
            return expr;
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
        start++;
        expr.nonOperationalResult = std::make_shared<rbc_value>(parseList(program, tlist, start, err));
        return expr;
    }
    bst_operation<token> bst = make_bst(program, tlist, start, err, br, false, obj);

    if (err->trace.ec)
        return expr;
    if (lineEnd)
    {
        if (start + 1 >= tlist.size())
            EXPR_ERROR_R(RS_EOF_ERROR, "Missing semicolon.", tlist.back().trace, expr);
        if (tlist.at(start + 1).type != token_type::LINE_END)
            EXPR_ERROR_R(RS_EOF_ERROR, "Missing semicolon.", tlist.at(start + 1).trace, expr);
        start++;
    }
    else if (!bst.isSingular())
        start++;
    if(prune)
    {
        prune_expr(program, bst, err);
        if (err->trace.ec)
            return expr;
    }
    expr.operation = bst;

    return expr;
}
#pragma endregion expressions
#undef EXPR_ERROR
#undef COMP_ERROR