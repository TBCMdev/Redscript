#pragma once
#include <memory>
#include <vector>
#include "rbc_value.hpp"

#include "../token.hpp"
#include "../bst.hpp"

struct rbc_program; // included in cpp


// holds a bst for pruning and computing,
// or a shared ptr to a raw non operational result such as an object.
// this ptr is not given a value to singleton expressions, such as integers or strings.
// only to values that cannot be operated on.
struct rs_expression
{
    using _ResultT = rbc_value;
    bst_operation<token> operation;
    bool macro = false;
    std::shared_ptr<_ResultT> nonOperationalResult = nullptr;
    
    _ResultT rbc_evaluate(rbc_program&,
                            bst_operation<token>* = nullptr,
                            bool = true,
                            std::shared_ptr<std::vector<std::shared_ptr<rbc_register>>> = nullptr);
};
