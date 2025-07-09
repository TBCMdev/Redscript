#pragma once
#include <vector>
#include <memory>
#include <variant>

#include "var_access.hpp"

template<typename _Storage>
struct bst_node
{
    std::vector<std::shared_ptr<bst_node<_Storage>>> children;
    bst_node* parent;

    std::shared_ptr<_Storage> value;

    inline std::shared_ptr<bst_node<_Storage>>& makeChild(_Storage& _child)
    {
        bst_node<_Storage> child;

        child.value = std::make_shared<_Storage>(_child);
        child.parent = this;
        std::shared_ptr<bst_node<_Storage>> ptr = std::make_shared<bst_node<_Storage>>(_child);
    
        children.push_back(ptr);

        return ptr;
    }
};
enum class bst_operation_type
{
    ADD,SUB,
    MUL,DIV,
    MOD,XOR, POW,

    NONE
};
enum class comparison_operation_type
{
    EQ,NEQ,GT,LT,GTE,LTE,

    NONE
};

std::string operationTypeToStr(bst_operation_type t);
inline int operatorPrecedence(bst_operation_type ot)
{
    switch(ot)
    {
        case bst_operation_type::XOR:
            return 0;
        case bst_operation_type::MOD:
            return 1;
        case bst_operation_type::MUL:
            return 2;
        case bst_operation_type::DIV:
            return 3;
        case bst_operation_type::ADD:
            return 4;
        case bst_operation_type::SUB:
            return 5;
        default:
            return -1;
    }
}
int operatorPrecedence(char op);
template<typename _T>
inline _T operator_compute(_T left, bst_operation_type op, _T right)
{
    using ot = bst_operation_type;
    switch(op)
    {
        case ot::ADD:
            return left + right;
        case ot::SUB:
            return left - right;
        case ot::MUL:
            return left * right;
        case ot::DIV:
            return left / right;
        case ot::MOD:
            if constexpr (std::is_integral_v<_T>)
            {
                return left % right;
            }
            break;
        default:
            break;
    }
    return left;
}

template<typename _Storage>
struct bst_operation
{
    // an operation can involve accessing variables such as x[1].b.a[3]
    using _NodeT  = std::variant<_Storage, rs_var_access_path>;
    using _ValueT = std::variant<bst_operation<_Storage>, _NodeT>;

    std::shared_ptr<_ValueT> left;
    std::shared_ptr<_ValueT> right;
    bst_operation_type operation = bst_operation_type::NONE;
    inline void makeSingular(_Storage& s)
    {
        left = std::make_shared<_ValueT>(s);

        right.reset();
        operation = bst_operation_type::NONE;
    }
    inline bool isSingular()
    {
        return left && left->index() && !right && operation == bst_operation_type::NONE;
    }
    inline bool assignNext(_Storage& s)
    {
        // TODO FIX
        if (right) return false;

        auto val = std::make_shared<_ValueT>(s);

        if(left)
            right = std::make_shared<_ValueT>(s);
        else
            left = std::make_shared<_ValueT>(s);

        return true;
    }
    inline bool assignNext(const rs_var_access_path& s)
    {
        if (right) return false;

        auto val = std::make_shared<_ValueT>(s);

        if(left)
            right = std::make_shared<_ValueT>(s);
        else
            left = std::make_shared<_ValueT>(s);

        return true;
    }
    inline bool assignNext(bst_operation<_Storage>& s)
    {
        if (right) return false;

        if(left)
            right = std::make_shared<_ValueT>(s);
        else
            left = std::make_shared<_ValueT>(s);

        return true;
    }
    inline bool setOperation(char c)
    {
        switch(c)
        {
            case '+':
                operation = bst_operation_type::ADD;
                break;
            case '-':
                operation = bst_operation_type::SUB;
                break;
            case '*':
                operation = bst_operation_type::MUL;
                break;
            case '/':
                operation = bst_operation_type::DIV;
                break;
            case '%':
                operation = bst_operation_type::MOD;
                break;
            case '^':
                operation = bst_operation_type::XOR;
                break;
            default:
                return false;
        }
        return true;
    }
    inline std::string tostr(int d = 0)
    {
        std::string tabs;
        for (int i = 0; i < d; i++) tabs.push_back('\t');

        std::string ret = '\n' + tabs + "< ";
        _ValueT& l = *left;
        if(l.index())
            ret += std::get<1>(l);
        else
            ret += std::get<0>(l).tostr(d + 1);
        if(operation != bst_operation_type::NONE)
        {
            ret += ", ";
            switch(operation)
            {
                case bst_operation_type::ADD:
                    ret.push_back('+');
                    break;
                case bst_operation_type::SUB:
                    ret.push_back('-');
                    break;
                case bst_operation_type::MUL:
                    ret.push_back('*');
                    break;
                case bst_operation_type::DIV:
                    ret.push_back('/');
                    break;
                case bst_operation_type::MOD:
                    ret.push_back('%');
                    break;
            }

        }
        else 
        {
            ret += ", NONE";
        }
        if(right)
        {
            ret += ", ";
            _ValueT& r = *right;
            if(r.index())
                ret += std::get<1>(r);
            else
                ret += std::get<0>(r).tostr(d + 1);
        }
        ret += ">";
        return ret;
    }
};

template<typename _Storage>
using bst = bst_node<_Storage>;

