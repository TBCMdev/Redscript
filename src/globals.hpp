#pragma once
#include "config.hpp"

#define RS_CONFIG_LOCATION "./rs.config"

#define RS_STORAGE_NAME "redscript"
#define RS_PROGRAM_STORAGE RS_STORAGE_NAME ":_program"

#define RS_PROGRAM_STACK     "stack"
#define RS_PROGRAM_DATA      "_internal"
#define RS_PROGRAM_VARIABLES "variables"
#define RS_PROGRAM_REGISTERS "registers"
#define RS_PROGRAM_RETURN_REGISTER "ret"
#define RS_PROGRAM_RETURN_TYPE_REGISTER "ret_type"
#define RBC_REGISTER_PLAYER "_CPU"
#define RBC_REGISTER_PLAYER_OBJ "alu"
#define RBC_COMPARISON_RESULT_REGISTER "cmp"
#define MC_DATAPACK_FOLDER "datapacks"
#define MC_MCMETA_FILE_NAME "pack.mcmeta"
#define MC_TEMP_STORAGE_NAME "temp"

/*
All types that can be a parameter of a byte-code assembly instruction.
- constants, which dont need to be referenced and can be copied.
- registers, operable & non operable, used for primarily arithmetic
- objects, either JSON-inline objects or class type object instances. TODO change from rs_object to rs_object_instance
- lists, lists
- void*, weird to have but are used to pass critical info to an instruction.
    example:
        a function call: sometimes a function call can exist somewhere where it is impossible to find,
        like x(){y(){z(){}w(){}}}
        if I call w() inside x, how does the instruction find w efficiently? it could be a global function,
        a function from a module, etc.
        so we use void* to pass the function* through to the instruction to save time.
- variable usage paths: x[0][2][3].b.a[1].c
*/
#define RBC_VALUE_T std::variant<rbc_constant,       \
                      std::shared_ptr<rbc_register>, \
                      std::shared_ptr<rs_variable>,  \
                      std::shared_ptr<rs_object>,    \
                      std::shared_ptr<rs_list>,      \
                      std::shared_ptr<void>,         \
                      rs_var_access_path>
#define RS_PROGRAM_DATA_DEFAULT "{\"" RS_PROGRAM_VARIABLES "\":[], \"" RS_PROGRAM_REGISTERS "\":[], \"" RS_PROGRAM_DATA "\":{}, \"" RS_PROGRAM_STACK "\":[], \"" RS_PROGRAM_RETURN_REGISTER "\": 0, \"temp\": 0}"

inline rs_config RS_CONFIG;