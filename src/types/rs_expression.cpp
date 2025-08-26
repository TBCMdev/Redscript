#include "rs_expression.hpp"

#include "rbc_value.hpp"
#include "rbc_register.hpp"

#include "../rbc.hpp"
#include "../errors.hpp"

#include <utility>


#pragma region expressions

rs_expression::_ResultT rs_expression::rbc_evaluate(rbc_program& program,
                                                    bst_operation<token>* node,
                                                    bool rootCaller, // for freeing registers at end of recursion
                                                    std::shared_ptr<std::vector<std::shared_ptr<rbc_register>>> usedRegisters)
{
    using _NodeT  = bst_operation<token>;
    using _ValueT = bst_operation<token>::_NodeT;

    if (nonOperationalResult)
        return *nonOperationalResult;
    if (!node) node = &operation;

    if (!usedRegisters)
        usedRegisters = std::make_shared<std::vector<std::shared_ptr<rbc_register>>>();

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
        auto lresult = rbc_evaluate(program, &std::get<_NodeT>(*node->left), false, usedRegisters);
        if (lresult.index() != 1)
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
        auto rresult = rbc_evaluate(program, &std::get<_NodeT>(*node->right), false, usedRegisters);
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

    bool occupy     = true;
    bool occupyLeft = true;

    if (leftVal->index() == 1)
    {
        reg = std::get<1>(*leftVal);
        if (!reg->vacant) 
        {
            reg = program.getFreeRegister(operableRegister);
            if (!reg)
                reg = program.makeRegister(operableRegister);
        } else occupy = false; // use register from prev operation to store this operation
    } else
    {
        reg = program.getFreeRegister(operableRegister);
        if (!reg)
            reg = program.makeRegister(operableRegister);
    }

    reg->vacant = false;
    usedRegisters->push_back(reg);

    size_t index = leftVal->index();
    size_t rindex = rightVal->index();

    if (operatorIsCommutative(node->operation))
    {
        // make sure variables are the ones being stored (lhs)
        if (index == 0 || index == 1) // constant or register
            occupyLeft = false;

    }
    
    if (index == 1 && rindex == 1)
        occupy = false;
    
    if (occupy)
        program (rbc_commands::registers::occupy(reg, occupyLeft ?  *leftVal  : *rightVal));
    program (rbc_commands::registers::operate(reg, occupyLeft ? *rightVal : *leftVal, static_cast<uint>(node->operation)));
    
    if (rootCaller)
        for(auto& reg : *usedRegisters)
            reg->free();

    return reg;
}
#pragma endregion expressions
#undef EXPR_ERROR
#undef COMP_ERROR