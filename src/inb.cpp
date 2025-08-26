#include "inb.hpp"
#include "mc.hpp"
#include "mchelpers.hpp"
#include "rbc.hpp"

// for readability
#ifndef INB_IMPL_PARAMETERS
#define INB_IMPL_PARAMETERS rbc_program& program, conversion::CommandFactory& factory, std::vector<rbc_value>& parameters, std::string& err 
#endif


#define IMPL_ERROR(msg) {err=msg " (impl errors do not have trace as of beta, check function calls)"; return nullptr;}
namespace inb_impls
{
    // technically tellraw impl.
    _InbRetT msg(INB_IMPL_PARAMETERS)
    {

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
                // rbc_register& reg = *std::get<1>(val);

                // factory.create_and_push(MC_TELLRAW_CMD_ID, MC_TELLRAW_OPERABLE_REGISTER());

                break;
            }
            case 2:
            {
                rs_variable& var = *std::get<2>(val);
                factory.create_and_push(MC_TELLRAW_CMD_ID, MC_TELLRAW_VARIABLE(_const.val, var));
                break;
            }
            case 4:
            {
                IMPL_ERROR("msg() does not allow raw list constants to be printed. Store the constant in a variable first.");
                break;
            }
            case 6:
            {
                rs_var_access_path& path = std::get<6>(val);

                factory.create_and_push(MC_TELLRAW_CMD_ID, MC_TELLRAW_VARIABLE_PATH(_const.val, path.toCompiledPath()));
                break;
            }
            default:
                IMPL_ERROR("tellraw does not accept these parameter types in this version.");
        }
        return nullptr;
        
    }
    _InbRetT kill(INB_IMPL_PARAMETERS)
    {
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
        return nullptr;
    }
    _InbRetT compile_assert(INB_IMPL_PARAMETERS)
    {
        err = "Thise inbuilt function is not implemented yet.";
        return nullptr;
    }

    namespace debug
    {
        _InbRetT print(INB_IMPL_PARAMETERS)
        {
            // todo
            msg(program, factory, parameters, generics, err);
            return nullptr;
        }
    }
    namespace time
    {
        _InbRetT now(INB_IMPL_PARAMETERS)
        {
            return std::make_shared<_InbRetV>(mc_command(false, MC_TIME_CMD_ID, "query gametime"));
        }
    }
}