#pragma once
#include "config.hpp"

#define RS_CONFIG_LOCATION "./rs.config"


#define RS_IMPL_LIBRARY_NAME "__redscript_impl"
#define RS_STORAGE_NAME "redscript"
#define RS_PROGRAM_STORAGE RS_STORAGE_NAME ":_program"

#define RS_PROGRAM_STACK     "stack"
#define RS_PROGRAM_PARAMETERS_RAW "parameters"
#define RS_PROGRAM_PARAMETERS RS_PROGRAM_STACK "[0]." RS_PROGRAM_PARAMETERS_RAW
#define RS_PROGRAM_DATA      "_internal"
#define RS_PROGRAM_VARIABLES_RAW "variables"
#define RS_PROGRAM_VARIABLES RS_PROGRAM_STACK "[0]." RS_PROGRAM_VARIABLES_RAW
#define RS_PROGRAM_REGISTERS "registers"
#define RS_PROGRAM_RETURN_REGISTER "ret"
#define RS_PROGRAM_RETURN_TYPE_REGISTER "ret_type"
#define RBC_REGISTER_PLAYER "_CPU"
#define RBC_REGISTER_PLAYER_OBJ "alu"
#define RBC_COMPARISON_RESULT_REGISTER "cmp"
#define MC_DATAPACK_FOLDER "datapacks"
#define MC_MCMETA_FILE_NAME "pack.mcmeta"
#define MC_TEMP_STORAGE_NAME "temp"


#define RS_DEFAULT_STACK_FRAME "{\"" RS_PROGRAM_VARIABLES_RAW "\": [], \"" RS_PROGRAM_PARAMETERS_RAW "\": {}}"

#define RS_PROGRAM_DATA_DEFAULT "{\"" RS_PROGRAM_REGISTERS "\":[], \"" RS_PROGRAM_DATA "\":{}, \"" RS_PROGRAM_STACK "\":[" RS_DEFAULT_STACK_FRAME "], \"" RS_PROGRAM_RETURN_REGISTER "\": 0, \"temp\": 0}"

struct rbc_parser;

inline rs_config RS_CONFIG;
inline rbc_parser* RS_PARSER_INSTANCE = nullptr;