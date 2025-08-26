#pragma once
#include "globals.hpp"
#include "types/rs_variable.hpp"

#define SEP " "
#define PADL(x) " " #x
#define PADR(x) #x " "
#define PAD(x) PADL(x) " "
#define INS(x) +(x)+
#define INS_L(x) +(x)
#define INS_R(x) (x)+
#define STR(x) std::to_string(x)
#define ARR_AT(arr, i) arr "[" INS(i) "]"

#define MC_DATA(cmd, where) #cmd SEP RS_PROGRAM_STORAGE SEP where
#pragma region variables

#define RS_PROGRAM_VARIABLES_SPECIFIC(stackID) RS_PROGRAM_STACK "[" INS(STR(stackID)) "]." RS_PROGRAM_VARIABLES_RAW
#define RS_PROGRAM_PARAMETERS_SPECIFIC(stackID) RS_PROGRAM_STACK "[" INS(STR(stackID)) "]." RS_PROGRAM_PARAMETERS_RAW

#define VAR_ID(x) STR(x.comp_info.varIndex)




#define MC_VARIABLE_VALUE(var) getVariableValueLocation(var)
#define MC_VARIABLE_TYPE(var) getVariableValueLocation(var)
#define MC_VARIABLE_VALUE_FULL(var) RS_PROGRAM_STORAGE SEP INS_L(MC_VARIABLE_VALUE(var))
#define MC_VARIABLE_TYPE_FULL(var) RS_PROGRAM_STORAGE SEP MC_VARIABLE_TYPE(var)
#define MC_GET_VARIABLE_VALUE(var) MC_DATA(get storage, INS_L(MC_VARIABLE_VALUE(var)))

#define MC_VARIABLE_JSON(data, scope, type) "{\"value\":" data ",\"scope\":" INS(scope) ",\"type\":" INS(type) "}"
#define MC_VARIABLE_JSON_DEFAULT(scope, type) MC_VARIABLE_JSON("0", scope, type)
#define MC_VARIABLE_JSON_VAL(data, scope, type) MC_VARIABLE_JSON(INS(data), scope, type)

#define MC_PARAMETER_JSON(id, data) "{" INS(id) ": " data "}"
#define MC_PARAMETER_JSON_DEFAULT(id) MC_PARAMETER_JSON(id, "0")
#define MC_PARAMETER_JSON_VAL(id, data) MC_PARAMETER_JSON(id, INS(data))

#define MC_VARIABLE_SET_CONST(var, v) PADR(modify storage) RS_PROGRAM_STORAGE SEP INS(MC_VARIABLE_VALUE(var)) PADR(set value) INS_L(v)
#define MC_VARIABLE_PATH_SET_CONST(path, v) PADR(modify storage) RS_PROGRAM_STORAGE SEP INS(path) SEP PADR(set value) INS_L(v)
#pragma endregion variables

#pragma region registers
#define MC_COMPARE_REG(id) "cmp" INS_L(STR(id))
#define MC_COMPARE_REG_FULL(id) RBC_REGISTER_PLAYER " cmp" INS_L(STR(id))

#define MC_OPERABLE_REG_RAW(id) "r" id
#define MC_OPERABLE_REG(id) RBC_REGISTER_PLAYER SEP MC_OPERABLE_REG_RAW(id)
#define MC_CREATE_OPERABLE_REG(id, criteria) PADR(objectives add) MC_OPERABLE_REG_RAW(INS(STR(id))) SEP criteria SEP "\"" INS(STR(id)) "\""
#define MC_COMPARE_REG_GET(id) PADR(players get) RBC_REGISTER_PLAYER SEP RBC_COMPARISON_RESULT_REGISTER INS_L(STR(id))
#define MC_COMPARE_REG_SET(id, val) PADR(players set) RBC_REGISTER_PLAYER SEP RBC_COMPARISON_RESULT_REGISTER INS(STR(id)) SEP val
#define MC_OPERABLE_REG_SET(id, v) PADR(players set) MC_OPERABLE_REG(INS(STR(id))) SEP INS_L(v)
#define MC_OPERABLE_REG_GET(id) PADR(players get) MC_OPERABLE_REG(INS_L(STR(id)))
#define MC_OPERABLE_REG_COPY(dest, src) PADR(players operation) RBC_REGISTER_PLAYER SEP MC_OPERABLE_REG_RAW(INS(STR(dest))) PAD(=) RBC_REGISTER_PLAYER SEP MC_OPERABLE_REG_RAW(INS_L(STR(src)))
#define MC_NOPERABLE_REG_GET(id) MC_DATA(get storage, ARR_AT(RS_PROGRAM_REGISTERS, STR(id)))
#define MC_NOPERABLE_REG(id) RBC_REGISTER_PLAYER SEP ARR_AT(RS_PROGRAM_REGISTERS, STR(id))
#pragma endregion registers

#pragma region operable_math
#define MC_REG_INCREMENT_CONST(id, x) PADR(players) PADR(add) MC_OPERABLE_REG(INS(STR(id))) SEP INS_L(x)
#define MC_REG_DECREMENT_CONST(id, x) PADR(players) PADR(remove) MC_OPERABLE_REG(INS(STR(id))) SEP INS_L(x)
#define MC_REG_OPERATE(lh_id, op_str, rh_id) PADR(players) PADR(operation) MC_OPERABLE_REG(INS(STR(lh_id))) SEP INS(op_str) SEP MC_OPERABLE_REG(INS_L(STR(rh_id)))
#pragma endregion operable_math

#pragma region stack
#define MC_STACK_PUSH_CONST(x) MC_DATA(modify storage, RS_PROGRAM_STACK) PAD(append value) INS_L(x)
#define MC_STACK_AT(id) ARR_AT(RS_PROGRAM_STACK, STR(id))
#define MC_GET_STACK_VALUE(id) MC_DATA(get storage, MC_STACK_AT(id))
#pragma endregion stack

#pragma region tellraw
#define MC_TELLRAW_CONST(selector, val) '@' INS(selector) SEP INS_L(val)

// DONT USE: not finished
#define MC_TELLRAW_OPERABLE_REGISTER(selector, id) '@' INS(selector) SEP "[{\"score\":{\"" MC_OPERABLE_REG(id) ".value\"}, {\"storage\":\"" RS_PROGRAM_DATA "\"}"
// for tellraw in particular a function needs to be made. Coming in next version.
#define MC_TELLRAW_VARIABLE(selector, var) '@' INS(selector) SEP "[{\"nbt\":\"" INS(getVariableValueLocation(var)) "\", \"storage\":\"" RS_PROGRAM_STORAGE "\"}]"
#define MC_TELLRAW_VARIABLE_PATH(selector, path) '@' INS(selector) SEP "[{\"nbt\":\"" INS(path) "\", \"storage\":\"" RS_PROGRAM_STORAGE "\"}]"
#pragma endregion tellraw

#pragma region mcmeta
#define MC_MCMETA_CONTENT(version) "{\"pack\": {\"pack_format\": " INS(STR(version)) ", \"description\": \"A program created by redscript.\"}}"
#pragma endregion mcmeta


#pragma region inbuilt
#define MC_KILL(selector) '@' + selector
#pragma endregion inbuilt

#pragma region conditionals
#define MC_COMPARE_REG_GET_RAW(id) RBC_REGISTER_PLAYER SEP RBC_COMPARISON_RESULT_REGISTER id
#define MC_COMPARE_RESET(id) MC_COMPARE_REG_SET(id, "0")
#define MC_COMPARE_EQ(id, lhs, rhs) op PAD(data) lhs SEP rhs PAD(run scoreboard) MC_COMPARE_REG_SET(id, "1")
#define MC_COMPARE_NEQ(id, lhs, rhs) op PAD(data) lhs SEP rhs PAD(run scoreboard) MC_COMPARE_REG_SET(id, "0")
#define MC_CREATE_COMPARISON_REGISTER(id, criteria) PADR(objectives add) "cmp" INS(STR(id)) SEP criteria SEP "\"cmp" INS(STR(id)) "\""

#pragma endregion conditionals

#pragma region temporary_storage
#define MC_TEMP_STORAGE RS_PROGRAM_STORAGE SEP MC_TEMP_STORAGE_NAME
#define MC_TEMP_SCOREBOARD_STORAGE RBC_REGISTER_PLAYER SEP MC_TEMP_STORAGE_NAME
#define MC_TEMP_STORAGE_SET_CONST(val) MC_DATA(modify storage, MC_TEMP_STORAGE_NAME) PAD(set value) INS_L(val)
#define MC_TEMP_STORAGE_SCOREBOARD_SET_CONST(val) PADR(players set) MC_TEMP_SCOREBOARD_STORAGE SEP INS_L(val)
#define MC_TEMP_STORAGE_SCOREBOARD_SET_RAW_CONST(val) PADR(players set) MC_TEMP_SCOREBOARD_STORAGE SEP val
#pragma endregion temporary_storage