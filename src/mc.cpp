#include "mc.hpp"
#include "util.hpp"
mc_command::_This mc_command::addroot()
{
    std::string name = "";

    switch (cmd)
    {
    case MC_KILL_CMD_ID:
        name = "kill";
        break;
    case MC_EXEC_CMD_ID:
        name = "execute";
        break;
    case MC_FUNCTION_CMD_ID:
        name = "function";
        break;
    case MC_DATA_CMD_ID:
        name = "data";
        break;
    case MC_SCOREBOARD_CMD_ID:
        name = "scoreboard";
        break;
    case MC_TELLRAW_CMD_ID:
        name = "tellraw";
        break;
    case MC_RETURN_CMD_ID:
        name = "return";
        break;
    default:
        WARN("Unknown command.");
        break;
    }
    body = name + ' ' + body;
    return THIS;
}
mc_command::_This mc_command::ifcmpreg(comparison_operation_type t, int rid)
{
    std::string k;
    if (body.starts_with("if") || isexec()) // dont add run keyword
        k = " matches 1 ";
    else
        k = " matches 1 run ";
    if (!isexec())
    {
        addroot();
        cmd = MC_EXEC_CMD_ID;
    }
    if (t != comparison_operation_type::EQ && t != comparison_operation_type::NEQ)
        WARN("Performing undefined operation on comparison register. Defaulting to neq.");
    std::string s = t == comparison_operation_type::EQ ? "if" : "unless";
    
    body = s + " score " MC_COMPARE_REG_GET_RAW(INS_L(STR(rid))) + k + body;
    return THIS;
}
mc_command::_This mc_command::ifcmp(const std::string &lhs, comparison_operation_type t, const std::string &rhs, bool negate)
{
    using T = comparison_operation_type;

    // TODO: no k impl (see other comparison functions)
    if (!isexec())
    {
        addroot();
        cmd = MC_EXEC_CMD_ID;
    }
    switch (t)
    {
    case T::EQ:
    {
        body = (negate ? "unless data " : "if data ") + lhs + SEP + rhs + " run " + body;
        break;
    }
    case T::NEQ:
    {
        body = (negate ? "if data " : "unless data ") + lhs + SEP + rhs + " run " + body;
        break;
    }
    default:
    {
        ERROR("Cannot perform integer comparison on non integer-like candidates.");
        break;
    }
    }
    return THIS;
}
mc_command::_This mc_command::ifint(const std::string &lhs, comparison_operation_type t, const std::string &rhs, bool constant, bool negate)
{
    using T = comparison_operation_type;
    // execute if score _CPU r0 matches 0 <run> scoreboard players set _CPU cmp0 1

    std::string k = body.starts_with("if") || isexec() ? SEP : " run ";

    if (!isexec())
    {
        addroot();
        cmd = MC_EXEC_CMD_ID;
    }

    std::string op = "=";
    if (!constant)
    {

        switch (t)
        {
        case T::EQ:
        case T::NEQ:
            break;
        case T::GT:
            op = ">";
            break;
        case T::GTE:
            op = ">=";
            break;
        case T::LT:
            op = "<";
            break;
        case T::LTE:
            op = "<=";
            break;
        default:
            WARN("Unknown int comparison operator.");
            break;
        }
    }
    else
        op = "matches";
    body = (negate ? "unless score " : "if score ") + lhs + SEP + op + SEP + rhs + k + body;
    return THIS;
}
mc_command::_This mc_command::store(bool result, const std::string &where)
{
    switch (cmd)
    {
    case MC_EXEC_CMD_ID:
    {
        if (result)
            body = "store result " + where + body;
        else
            body = "store success " + where + body;

        break;
    }
    default:
    {
        addroot();
        if (result)
            body = "store result " + where + " run " + body;
        else
            body = "store success " + where + " run " + body;
        cmd = MC_EXEC_CMD_ID;
        break;
    }
    }
    return THIS;
}
mc_command::_This mc_command::storeSuccess(const std::string &where)
{
    return store(false, where);
}
mc_command::_This mc_command::storeSuccess(const std::string &where, const std::string &dataType, int scale)
{
    return store(false, where + " " + dataType + " " + std::to_string(scale));
}
mc_command::_This mc_command::storeResult(const std::string &where, const std::string &dataType, int scale)
{
    return store(true, where + " " + dataType + " " + std::to_string(scale));
}
mc_command::_This mc_command::storeResult(const std::string &where)
{
    return store(true, where);
}
const std::filesystem::path makeDatapack(const std::filesystem::path &path)
{
    const std::filesystem::path funcDir = path / "data" / RS_STORAGE_NAME / "function";
    std::filesystem::create_directories(funcDir);

    std::ofstream mcMetaPack(path / MC_MCMETA_FILE_NAME);

    if (!RS_CONFIG.exists("versionid"))
        throw std::runtime_error("'versionid' is not specified in redscript config.");

    const int version = RS_CONFIG.get<int>("versionid");

    mcMetaPack << MC_MCMETA_CONTENT(version);

    mcMetaPack.close();

    return funcDir;
}
std::shared_ptr<comparison_register> mc_program::getFreeComparisonRegister()
{
    for (std::shared_ptr<comparison_register> reg : comparisonRegisters)
    {
        if (reg->vacant)
            return reg;
    }
    return nullptr;
}
void writemc(mc_program &program, std::string name, const std::string &path, std::string &err)
{
    if (!RS_CONFIG.exists("mcpath"))
    {
        err = "'mcpath' doesn't exist in config. Can't write.";
        return;
    }

    toLower(name);

    auto writeFunction = [&](mc_function &func, const std::filesystem::path &path)
    {
        std::ofstream stream(path);
        for (auto &command : func.commands)
            stream << command.body << '\n';
        stream.close();
        return true;
    };
    try
    {
        std::filesystem::path mcpath(RS_CONFIG.get<std::string>("mcpath"));
        if (!name.ends_with(".mcfunction"))
            name += ".mcfunction";
        const std::filesystem::path safeName = removeSpecialCharacters(name);
        mcpath = mcpath / path;
        if (!std::filesystem::exists(mcpath))
        {
            err = std::format("The Minecraft world located at '{}' doesn't exist.", mcpath.string());
            return;
        }
        mcpath /= MC_DATAPACK_FOLDER / (safeName.stem());
        const std::filesystem::path funcPath = makeDatapack(mcpath);

        std::filesystem::path to = funcPath / safeName;
        if (!writeFunction(program.globalFunction, to))
        {
        _error:
            err = std::format("Could not write function to '{}'.", to.string());
            return;
        }
        for (auto &function : program.functions)
        {

            std::filesystem::path parentHashPath;

            if (function.modulePath.size() == 0)
            {
                if (function.parentalHashStr.empty())
                    to = funcPath / (function.name + ".mcfunction");
                else
                    to = funcPath / (function.parentalHashStr + '_' + function.name + ".mcfunction");
            }
            else
            {
                to = funcPath;
                for(std::string& _module : function.modulePath)
                {
                    to /= _module;
                    // maybe trying to create way too many directories.
                    // TODO: fix!
                    std::filesystem::create_directory(to);
                }

                if (function.parentalHashStr.empty())
                    to /= (function.name + ".mcfunction");
                else
                    to /= (function.parentalHashStr + '_' + function.name + ".mcfunction");
            }
            if (!writeFunction(function, to))
                goto _error;
        }
        // TODO: other functions
    }
    catch (std::exception &error)
    {
        err = error.what();
    }
}
