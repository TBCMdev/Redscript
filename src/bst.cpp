#include "bst.hpp"

std::string operationTypeToStr(bst_operation_type t)
{
    switch(t)
    {
        case bst_operation_type::ADD:
            return std::string(1, '+');
        case bst_operation_type::SUB:
            return std::string(1, '-');
        case bst_operation_type::MUL:
            return std::string(1, '*');
        case bst_operation_type::DIV:
            return std::string(1, '/');
        case bst_operation_type::POW:
            return "**";
        case bst_operation_type::XOR:
            return std::string(1, '^');
        case bst_operation_type::MOD:
            return std::string(1, '%');
        default:
            break;
    }

    return "NULL";
}

comparison_operation_type _binary_simplify_tt(token_type tt, bool negate)
{
    switch(tt)
    {
        case token_type::COMPARE_EQUAL:
            return negate ? comparison_operation_type::NEQ : comparison_operation_type::EQ;
        // case token_type::COMPARE_NOTEQUAL: same thing
        default:
            return negate ? comparison_operation_type::EQ : comparison_operation_type::NEQ;
    }
}
comparison_operation_type _tt_to_cot(token_type tt)
{
    switch (tt)
    {
        case token_type::COMPARE_EQUAL:
            return comparison_operation_type::EQ;
        case token_type::COMPARE_NOTEQUAL:
            return comparison_operation_type::NEQ;
        case token_type::COMPARE_GREATER:
            return comparison_operation_type::GT;
        case token_type::COMPARE_LESS:
            return comparison_operation_type::LT;
        case token_type::COMPARE_GREATER_EQ:
            return comparison_operation_type::GTE;
        case token_type::COMPARE_LESS_EQ:
            return comparison_operation_type::LTE;
        default:
            return comparison_operation_type::NONE;
    }
}
bool operatorIsCommutative(bst_operation_type t)
{
    switch(t)
    {
        case bst_operation_type::ADD:
        case bst_operation_type::MUL:
        case bst_operation_type::XOR:
            return true;
        default:
            return false;
    }
}
int operatorPrecedence(char op)
{
    switch(op)
    {
        case '^':
            return 0;
        case '%':
            return 1;
        case '*':
            return 2;
        case '/':
            return 3;
        case '+':
            return 4;
        case '-':
            return 5;
        default:
            return -1;
    }
}
