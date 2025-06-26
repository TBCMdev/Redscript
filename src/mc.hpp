#pragma once
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <stack>
#include "error.hpp"
#include "bst.hpp"
#include "mchelpers.hpp"
#include "util.hpp"

#define MC_DATA_CMD_ID 0
#define MC_EXEC_CMD_ID 1
#define MC_FUNCTION_CMD_ID 2
#define MC_SCOREBOARD_CMD_ID 3
#define MC_TELLRAW_CMD_ID 4
#define MC_KILL_CMD_ID 5
#define MC_RETURN_CMD_ID 6
#define THIS *this;

typedef unsigned int uint;
struct rs_variable;
struct rbc_function;


struct mc_command
{
    using _This = mc_command&;

    bool        macro;
    uint        cmd;
    std::string body;

    constexpr inline bool isexec()
    { return cmd == MC_EXEC_CMD_ID; }

    _This addroot();

    _This store(bool result, const std::string& where);
    _This storeResult(const std::string& where, const std::string& dataType, int scale);
    _This storeResult(const std::string& where);
    _This storeSuccess(const std::string& where);
    _This storeSuccess(const std::string &where, const std::string &dataType, int scale);

    _This ifcmpreg(comparison_operation_type t, int id);
    _This ifcmp(const std::string& lhs, comparison_operation_type t, const std::string& rhs, bool negate = false);
    _This ifint   (const std::string& lhs, comparison_operation_type t, const std::string& rhs, bool constant, bool negate = false);
};

typedef std::vector<mc_command> mccmdlist;
struct mc_function
{
    std::string name;
    std::vector<mc_command> commands;
    std::vector<std::string> modulePath;
    std::string parentalHashStr = "";
};
struct comparison_register
{
    uint id;
    bool vacant = true;
    comparison_operation_type operation = comparison_operation_type::EQ;
    inline void free()
    {
        vacant = true;
    }
};
struct mc_program
{
    uint varStackCount = 0;
    iterable_stack<std::pair<int, std::shared_ptr<comparison_register>>> blocks;
    std::vector<std::shared_ptr<comparison_register>> comparisonRegisters;
    std::vector<mc_function> functions;
    mc_function* currentFunction = nullptr;
    // parameter name: parameter id
    std::vector<rs_variable*> stack;
    mc_function globalFunction;

    std::shared_ptr<comparison_register> getFreeComparisonRegister();
    
};
const std::filesystem::path makeDatapack(const std::filesystem::path&);
void writemc(mc_program&, std::string, const std::string&, std::string&);