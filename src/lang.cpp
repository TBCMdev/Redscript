#include "lang.hpp"
#include "rbc.hpp"
#define EXPR_ERROR_R(_ec, _message, _trace, _ret, ...)                                                       \
    {                                                                                                        \
        *err = rs_error(_message, *program.context->content, _trace, program.context->fName, ##__VA_ARGS__); \
        err->trace.ec = _ec;                                                                                 \
        return _ret;                                                                                         \
    }
#pragma region objects


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

// call after first [

#pragma endregion expressions
#undef EXPR_ERROR
#undef COMP_ERROR