#include "inb.hpp"
#include "rbc.hpp"
#include "lang.hpp"
#include "mchelpers.hpp"

// for readability
#ifndef INB_IMPL_PARAMETERS
#define INB_IMPL_PARAMETERS rbc_program& program, conversion::CommandFactory& factory, std::vector<rbc_value>& parameters, std::string& err 
#endif

#define IMPL_ERROR(msg) {err=msg " (impl errors do not have trace as of beta, check function calls)"; return;}
namespace inb_impls
{
    // technically tellraw impl.
    void msg(INB_IMPL_PARAMETERS)
    {

        (void) program; // todo remove


        // TODO: tellraw
        rbc_value& selector = parameters.at(0);
        if (selector.index() != 0)
        {
        fail:
            IMPL_ERROR("Expected selector as argument 0 for candidate (tellraw) impl::msg.");
        }
        rbc_constant& _const = std::get<0>(selector);
        if (_const.val_type != token_type::SELECTOR_LITERAL)
            goto fail;

        rbc_value& val = parameters.at(1);
        switch(val.index())
        {
            case 0:
            {
                rbc_constant& c = std::get<0>(val);
                
                factory.create_and_push(MC_TELLRAW_CMD_ID, MC_TELLRAW_CONST(_const.val, c.val));
                break;
            }
            case 1:
            {
                WARN("Non implemented tellraw functionality.");
                break;
            }
            case 2:
            {
                rs_variable& var = *std::get<2>(val);
                factory.create_and_push(MC_TELLRAW_CMD_ID, MC_TELLRAW_VARIABLE(_const.val, var.comp_info.varIndex));
                break;
            }
            default:
                IMPL_ERROR("tellraw does not accept these parameter types in this version.");
        }
        
    }
    void kill(INB_IMPL_PARAMETERS)
    {
        (void) program; // todo remove

        rbc_value& selector = parameters.at(0);
        if (selector.index() != 0)
        {
        fail:
            IMPL_ERROR("Expected selector as argument 0 for candidate (tellraw) impl::msg.");
        }
        rbc_constant& _const = std::get<0>(selector);
        if (_const.val_type != token_type::SELECTOR_LITERAL)
            goto fail;

        factory.create_and_push(MC_KILL_CMD_ID, MC_KILL(_const.val));
    }
    void compile_assert(INB_IMPL_PARAMETERS)
    {
        (void) program; // todo remove
        (void) factory;
        (void) parameters;

        err = "Thise inbuilt function is not implemented yet.";
    }
}