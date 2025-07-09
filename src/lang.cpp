#include "lang.hpp"
#include "rbc.hpp"
#define EXPR_ERROR_R(_ec, _message, _trace, ret, ...)                             \
    {                                                                    \
        program.context = new rs_error(_message, program.currentFragment->fileContent, _trace, std::make_shared<std::vector<std::string>>(program.callStackStr()), program.currentFragment->fileName, ##__VA_ARGS__);  \
        program.context->trace.ec = _ec;                                             \
        throw program.context;                                                     \
    }

#pragma region variable_access

std::string rs_var_access_path::toPath()
{
    std::stringstream stream;
    stream << fromVar->name;
    for(auto& p : segments)
    {
        if (p.isArray)
            stream << '[' << p.accessKey.val << ']';
        else
            stream << '.' << p.accessKey.val;
    }

    return stream.str();
}

#pragma endregion variable_access

#pragma region objects


#pragma endregion objects
#pragma region expressions

rs_expression::_ResultT rs_expression::rbc_evaluate(rbc_program& program, bst_operation<token>* node)
{
    using _NodeT  = bst_operation<token>;
    using _ValueT = bst_operation<token>::_NodeT;

    if (nonOperationalResult)
        return *nonOperationalResult;
    if (!node) node = &operation;

    const bool lSingle  = node->left->index();
    const bool rSingle  = node->right && node->right->index();


    std::shared_ptr<rbc_value> leftVal;
    std::shared_ptr<rbc_value> rightVal;

    // an operable computation is made when the left and right parts of the node contain integer values.
    // whether that be an integer or a variable holding an integer.
    // a quick fix is to set this = true, letting all computations become operable (except for ones with non operable registers)
    bool operableRegister     = true; // was false, error here. TODO fix

    if (!lSingle)
    {
        auto lresult = rbc_evaluate(program, &std::get<_NodeT>(*node->left));
        if (lresult.index())
            leftVal = std::make_shared<rbc_value>(std::get<1>(lresult));
        else
        {
            sharedt<rbc_register>& reg = std::get<sharedt<rbc_register>>(lresult);
            leftVal = std::make_shared<rbc_value>(reg);
            operableRegister = reg->operable;
        }
    }else 
    {
        _ValueT& value = std::get<_ValueT>(*node->left);
        std::shared_ptr<rs_variable> var;

        if (value.index() == 0)
        {
            token& tok = std::get<token>(value);
            if ((var = program.getVariable(tok)))
                leftVal = std::make_shared<rbc_value>(var);
            else
                leftVal = std::make_shared<rbc_value>(rbc_constant(tok.type, tok.repr,
                                                                std::make_shared<raw_trace_info>(tok.trace)));
        }else
            leftVal = std::make_shared<rbc_value>(std::get<rs_var_access_path>(value));
    }

    if(!node->right)
        return *leftVal;

    if (!rSingle)
    {
        auto rresult = rbc_evaluate(program, &std::get<_NodeT>(*node->right));
        if(rresult.index() != 1)
            rightVal = std::make_shared<rbc_value>(rresult);
        else
        {
            sharedt<rbc_register>& reg = std::get<sharedt<rbc_register>>(rresult);
            if (operableRegister && !reg->operable)
                EXPR_ERROR_R(RS_UNSUPPORTED_OPERATION_ERROR,
                    "Unsupported operation between operable and non operable register. If you see this particular message, flag an error on the github.",
                    stack_trace(), *leftVal);
            rightVal = std::make_shared<rbc_value>(reg);
        }

    }else 
    {
        _ValueT& value = std::get<_ValueT>(*node->right);
        std::shared_ptr<rs_variable> var;

        if (value.index() == 0)
        {
            token& tok = std::get<token>(value);
            if ((var = program.getVariable(tok)))
                rightVal = std::make_shared<rbc_value>(var);
            else
                rightVal = std::make_shared<rbc_value>(rbc_constant(tok.type, tok.repr,
                                                                std::make_shared<raw_trace_info>(tok.trace)));
        }else
            rightVal = std::make_shared<rbc_value>(std::get<rs_var_access_path>(value));
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

// call after first [

#pragma endregion expressions
#undef EXPR_ERROR
#undef COMP_ERROR