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
