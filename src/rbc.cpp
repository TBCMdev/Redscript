#include "rbc.hpp"
#include "lang.hpp"
#include "lexer.hpp"
#include "file.hpp"
#include "mchelpers.hpp"
#include <regex>

namespace rbc_commands
{
    namespace registers
    {
        rbc_command occupy(std::shared_ptr<rbc_register> reg, rbc_value val)
        {
            reg->vacant = false;
            return rbc_command(rbc_instruction::SAVE, reg, val);
        }
        rbc_command operate(std::shared_ptr<rbc_register> reg, rbc_value val, uint op)
        {
            return rbc_command(rbc_instruction::MATH, reg, val, rbc_constant(token_type::INT_LITERAL, std::to_string(op)));
        }
    }
    namespace variables
    {
        rbc_command set(std::shared_ptr<rs_variable> var, rbc_value val)
        {
            return rbc_command(rbc_instruction::SAVE, rbc_value(var), val);
        }
        rbc_command create(std::shared_ptr<rs_variable> var, rbc_value val)
        {
            return rbc_command(rbc_instruction::CREATE, rbc_value(var), val);
        }
        rbc_command storeReturn(std::shared_ptr<rs_variable> var)
        {
            return rbc_command(rbc_instruction::SAVERET, rbc_value(var));
        }
        rbc_command create(std::shared_ptr<rs_variable> var)
        {
            return rbc_command(rbc_instruction::CREATE, rbc_value(var));
        }
    }
}
rbc_function::rbc_function(const rbc_function& other, const std::vector<rs_type_info>& resolvedGenerics)
{
    name = other.name;
    scope = other.scope;
    decorators = other.decorators;
    instructions = other.instructions;
    modulePath = other.modulePath;
    parent = nullptr; // generic functions can't have parent
    generics = other.generics;
    assignedGenerics = std::make_shared<std::vector<rs_type_info>>(resolvedGenerics);
    hasBody = other.hasBody;

    // Deep copy parameters
    parameters.reserve(other.parameters.size());
    for (const auto& param : other.parameters) {
        auto newParam = std::make_shared<rs_variable>(*param);
        rs_type_info::resolveGenericsIn(newParam->type_info, resolvedGenerics);
        parameters.push_back(newParam);
    }

    // Deep copy return type
    if (other.returnType) {
        returnType = std::make_shared<rs_type_info>(*other.returnType);
        rs_type_info::resolveGenericsIn(*returnType, resolvedGenerics);
    }

    // Deep copy local variables
    for (const auto& [name, varPair] : other.localVariables) {
        const auto& [varPtr, isCaptured] = varPair;
        auto newVar = std::make_shared<rs_variable>(*varPtr);
        rs_type_info::resolveGenericsIn(newVar->type_info, resolvedGenerics);
        localVariables[name] = std::make_pair(newVar, isCaptured);
    }

    // Child functions (shallow copy)
    for (const auto& [childName, childFunc] : other.childFunctions) {
        childFunctions[childName] = childFunc;
    }
}
#pragma region decorators
rbc_function_decorator parseDecorator(const std::string& name)
{
    if (name == "extern") return rbc_function_decorator::EXTERN;
    if (name == "wrapper") return rbc_function_decorator::WRAPPER;
    if (name == "noreturn") return rbc_function_decorator::NORETURN;
    if (name == "__single__")  return rbc_function_decorator::SINGLE;
    if (name == "__cpp__") return rbc_function_decorator::CPP;
    if (name == "__nocompile__") return rbc_function_decorator::NOCOMPILE;
    return rbc_function_decorator::UNKNOWN;
}

#pragma endregion decorators
#pragma region operators

void rbc_program::operator()(std::vector<rbc_command>& instructions)
{
    if (currentFunction)
    {
        std::vector<rbc_command>& toAppend = currentFunction->instructions;
        toAppend.insert(toAppend.end(), instructions.begin(), instructions.end());
    }
    else
        globalFunction.instructions.insert(globalFunction.instructions.end(),
                                            instructions.begin(), instructions.end());
}
void rbc_program::operator ()(const rbc_command& instruction)
{
    if (currentFunction) currentFunction->instructions.push_back(instruction);
    else globalFunction.instructions.push_back(instruction);
}

#pragma endregion operators
std::string rbc_function::getParentHashStr()
{
    if (!parent) return "";

    std::shared_ptr<rbc_function> currentParent = parent;
    std::string parentHash;

    do
    {
        parentHash += util::hashToHex(currentParent->name) + '_';
    } while ((currentParent = currentParent->parent));
    
    parentHash.pop_back();

    return parentHash;
}
std::string rbc_function::getGenericsHashStr()
{
    if (!generics || !assignedGenerics) return "";

    std::string combined;
    for (rs_type_info& t : *assignedGenerics) {
        combined += t.tostr() + ';'; // delimiter to avoid ambiguity
    }

    // Optional: hash the whole string to get a compact ID
    return util::hashToHex(combined);
}
std::string rbc_function::toStr()
{
    std::stringstream stream;
    stream << "{name=" << name;

    int i = 1;
    for(auto& var : localVariables)
    {
        stream << ',';
        if(var.second.second) // parameter
        {
            stream << 'p' << i << '=' << var.second.first->tostr();
        }
    }
    return stream.str();
}
rs_variable* rbc_function::getParameterByName(const std::string& name)
{
    for(auto& var : parameters)
    {
        if (var->name == name)
            return var.get();
    }
    return nullptr;
}
rs_variable* rbc_function::getNthParameter(size_t p)
{
    if (p < parameters.size())
        return parameters.at(p).get();
    return nullptr;
}
std::string rbc_function::toHumanStr()
{
    std::stringstream stream;
    stream << name << ':';
    for(auto& instruction : instructions)
    {
        stream << '\n';
        stream << '\t' << instruction.toHumanStr();
    }

    return stream.str();
}
std::string rbc_command::tostr()
{
    std::stringstream stream;
    stream << '{' << static_cast<int>(type);
    for(auto& p : parameters)
    {
        switch(p->index())
        {
            case 0:
                stream << ", " << std::get<0>(*p).tostr();
                break;
            case 1:
                stream << ", " << std::get<1>(*p)->tostr();
                break;
            case 2:
                stream << ", " << std::get<2>(*p)->tostr();
                break;
            case 3:
                stream << " " << std::get<3>(*p)->tostr();
                break;
        }
    }
    stream << '}';
    return stream.str();
}
std::string rbc_command::toHumanStr()
{
    std::stringstream stream;
    switch(type)
    {
        case rbc_instruction::CALL:
            stream << "CALL ";
            break;
        case rbc_instruction::CREATE:
            stream << "CREATE ";
            break;
        case rbc_instruction::DEL:
            stream << "DEL ";
            break;
        case rbc_instruction::INC:
            stream << "INC";
            break;
        case rbc_instruction::DEC:
            stream << "DEC";
            break;
        case rbc_instruction::EQ:
            stream << "EQ ";
            break;
        case rbc_instruction::GT:
            stream << "GT ";
            break;
        case rbc_instruction::IF:
            stream << "IF ";
            break;
        case rbc_instruction::ELSE:
            stream << "ELSE";
            break;
        case rbc_instruction::ENDIF:
            stream << "ENDIF";
            break;
        case rbc_instruction::LT:
            stream << "LT ";
            break;
        case rbc_instruction::MATH:
            stream << "MATH ";
            break;
        case rbc_instruction::NEQ:
            stream << "NEQ ";
            break;
        case rbc_instruction::NIF:
            stream << "NIF ";
            break;
        case rbc_instruction::RET:
            stream << "RET ";
            break;
        case rbc_instruction::SAVE:
            stream << "SAVE ";
            break;
        case rbc_instruction::PUSH:
            stream << "PUSH ";
            break;
        case rbc_instruction::POP:
            stream << "POP ";
            break;
        default:
            stream << "UNKNOWN ";
            break;
    }
    int c = 0;
    for(auto& p : parameters)
    {
        if (c > 0)
            stream << ", ";
        switch(p->index())
        {
            case 0:
                stream << std::get<0>(*p).tostr();
                break;
            case 1:
                stream << std::get<1>(*p)->tostr();
                break;
            case 2:
                stream << std::get<2>(*p)->tostr();
                break;
            case 3:
                stream << std::get<3>(*p)->tostr();
                break;
        }
        c++;
    }
    return stream.str();
}
std::shared_ptr<rs_variable> rbc_program::getVariable(const std::string& name)
{
    auto result = std::find_if(globalVariables.begin(), globalVariables.end(),
    [&](std::shared_ptr<rs_variable>& var)
        {return var->name == name;}
    );
    if(result != globalVariables.end()) return *result;
    
    if (!currentFunction) return nullptr;

    auto nresult = currentFunction->localVariables.find(name);
    
    if (nresult != currentFunction->localVariables.end()
    && nresult->second.first->scope <= currentScope) return nresult->second.first;

    // also check parent functions

    if (functionStack.size() > 0)
    {
        for(auto it=functionStack.begin(); it != functionStack.end(); ++it)
        {
            std::shared_ptr<rbc_function>& v = *it;

            nresult = v->localVariables.find(name);

            if (nresult != v->localVariables.end())
                return nresult->second.first;
        }
    }

    return nullptr;
}
sharedt<rbc_register> rbc_program::getFreeRegister(bool operable)
{
    for(auto& reg : registers)
    {
        if (reg->vacant)
        {
            if (operable)
            {
                if (reg->operable) return reg;
            }
            else return reg;
        }
    }
    return nullptr;
}
sharedt<rbc_register> rbc_program::makeRegister(bool operable, bool vacant)
{
    uint id = registers.size();
    registers.push_back(std::make_shared<rbc_register>(id, operable, vacant));
    
    return registers[id];
}
#define PRE_PROCESS_ERROR(_ec, message, ...)                             \
    {                                                                    \
        *err = rs_error(message, content, current->trace, fName, current->trace.start, ##__VA_ARGS__);  \
        err->trace.ec = _ec;                                             \
        return;                                                    \
    }
void preprocess(token_list& tokens, std::string fName, std::string& content, rs_error* err, fragment_ptr_deque& fragments,
                std::shared_ptr<std::vector<std::filesystem::path>> visited)
{
    long long       _At = 0;
    const long long S   = tokens.size();
    std::filesystem::path rootPath = std::filesystem::absolute(fName);

    if(!visited)
        visited = std::make_shared<std::vector<std::filesystem::path>>();

    do
    {
        token* current = &tokens.at(_At);

        switch(current->type)
        {
            case token_type::KW_USE:
            {
                if (_At + 1 >= S) 
                    PRE_PROCESS_ERROR(RS_SYNTAX_ERROR, "Expected file to import, not EOF.");
                
                token& path = tokens.at(++_At);
                std::string file = (std::regex_replace(path.repr, std::regex("\\."), "/") + ".rsc");
                std::filesystem::path filePath = rootPath.parent_path() / file;
                if (visited && std::find(visited->begin(), visited->end(), filePath) != visited->end())
                    PRE_PROCESS_ERROR(RS_ALREADY_INCLUDED_ERROR, "This file has already been included.");
                visited->push_back(filePath);
                std::string fileContent = readFile(filePath);

                if (fileContent.empty())
                {
                    if (RS_CONFIG.exists("lib"))
                    {
                        std::filesystem::path libPath = std::filesystem::absolute(RS_CONFIG.get<std::string>("lib"));
                        filePath = libPath / file;

                        fileContent = readFile(filePath);

                        if (fileContent.empty())
                            PRE_PROCESS_ERROR(RS_SYNTAX_ERROR, "Could not find import '{}'.", path.repr);
                    }
                    else
                        PRE_PROCESS_ERROR(RS_SYNTAX_ERROR, "Could not find import '{}'.", path.repr);
                }
                if (path.type != token_type::WORD)
                    PRE_PROCESS_ERROR(RS_SYNTAX_ERROR, "Expected file name.");

                if (_At + 1 >= S || tokens.at(++_At).type != token_type::LINE_END)
                    PRE_PROCESS_ERROR(RS_SYNTAX_ERROR, "Missing semicolon.");
                std::string filePathStr = filePath.filename().string();
                token_list fileTokens = tlex(filePathStr, fileContent, err);

                preprocess(fileTokens, filePathStr, fileContent, err, fragments, visited);

                if(err->trace.ec)
                    return;
                // OLD: TODO REMOVE
                // const size_t offset = fileContent.length() + 1; // + 1 for \n
                // for(size_t i = 0; i < tokens.size(); i++)
                // {
                //     raw_trace_info& trace = tokens.at(i).trace;

                //     trace.at += offset;
                //     trace.nlindex += offset;
                // }
                // tokens.insert(tokens.begin(), fileTokens.begin(), fileTokens.end());

                // content = fileContent + '\n' + content;
                // _At += fileTokens.size();

                break;
            }
            default:
                break;
        }
    } while(++_At < S);

    fragments.push_back(std::make_shared<project_fragment>(fName, content, tokens));

}
#define RS_ASSERTC(C, m) if (!(C)) {err=m;return {};}
#define RS_ASSERT_SIZE(C) RS_ASSERTC(C, "Invalid byte code parameter count. This error is a bug, flag it on github.")
#define RS_ASSERT_SUCCESS if (!err.empty()) {return mcprogram;}
mc_program tomc(rbc_program& program, const std::string& moduleName, std::string& err)
{
    mc_program mcprogram;
    conversion::CommandFactory factory(mcprogram, program);
    
    auto parseFunction = [&](std::vector<rbc_command>& instructions) -> mccmdlist
    {
        for(size_t i = 0; i < instructions.size(); i++)
        {
            auto& instruction = instructions.at(i);
            const size_t size = instruction.parameters.size();

            switch(instruction.type)
            {
                case rbc_instruction::CREATE:
                {
                    RS_ASSERT_SIZE(size > 0);
                    rs_variable& var = *std::get<sharedt<rs_variable>>(*instruction.parameters.at(0));
                    if (instruction.parameters.size() == 1)
                        factory.createVariable(var);
                    else
                    {
                        rbc_value& val = *instruction.parameters.at(1);
                        factory.createVariable(var, val);
                    }
                    // RS_ASSERT_SUCCESS;
                    break;
                }
                case rbc_instruction::SAVE:
                {
                    RS_ASSERT_SIZE(size == 2);
                    rbc_value& reg = *instruction.parameters.at(0);
                    switch(reg.index())
                    {
                        // register
                        case 1:
                        {
                            rbc_register& regist = *std::get<sharedt<rbc_register>>(reg);
                            regist.vacant = false;
                            factory.setRegisterValue(regist,
                                                    *instruction.parameters.at(1));
                            break;
                        }
                        case 2:
                        {
                            factory.setVariableValue(*std::get<sharedt<rs_variable>>(reg), *instruction.parameters.at(1));
                        }
                    }
                    break;
                }
                case rbc_instruction::MATH:
                {
                    RS_ASSERT_SIZE(size > 2);
                    rbc_constant& val = std::get<rbc_constant>(*instruction.parameters.at(2));
                    bst_operation_type operation = bst_operation_type::NONE;
                    int operatorID = std::stoi(val.val);
                    switch(operatorID)
                    {
                        case 0:
                            operation = bst_operation_type::ADD;
                            break;
                        case 1:
                            operation = bst_operation_type::SUB;
                            break;
                        case 2:
                            operation = bst_operation_type::MUL;
                            break;
                        case 3:
                            operation = bst_operation_type::DIV;
                            break;
                        case 4:
                            operation = bst_operation_type::MOD;
                            break;
                        case 5:
                            operation = bst_operation_type::XOR;
                            break;
                        case 6:
                            operation = bst_operation_type::POW;
                            break;
                    }
                    factory.math(*instruction.parameters.at(0), *instruction.parameters.at(1), operation); 
                    break;
                }
                case rbc_instruction::CALL:
                {
                    RS_ASSERT_SIZE(size > 0);
                    std::shared_ptr<rbc_function> f = nullptr;

                    rbc_value& p0 = *instruction.parameters.at(0);
                    std::string name;
                    if (p0.index() == 0)
                    {
                        name = std::get<rbc_constant>(*instruction.parameters.at(0)).val;
                        rs_module* fromModule = nullptr;
                        if (size > 1)
                        {
                            fromModule = (rs_module*) std::get<std::shared_ptr<void>>(*instruction.parameters.at(1)).get();
                            
                            if(!fromModule)
                            {
                                err = "Function defined in module has caused seg fault. Flag this error on the github, it should not occur.";
                                break;
                            }
                            else
                                f = fromModule->functions.find(name)->second;
                        }
                        else
                            f = program.functions.find(name)->second;
                    }
                    else
                    {
                        std::shared_ptr<void> func = std::get<std::shared_ptr<void>>(p0);
                        f = std::static_pointer_cast<rbc_function>(func);
                        name = f->name;
                    }
                    rbc_function& func = *f;
                    factory.disableBuffer();
                    
                    if (std::find(func.decorators.begin(), func.decorators.end(), rbc_function_decorator::CPP) != func.decorators.end())
                    {
                        long caret = i;
                        std::vector<rbc_value> parameters;
                        rbc_command* cmd;

                        // we dont need the parameter instructions anymore.
                        factory.clearBuffer();
                        while(--caret >= 0 && (cmd = &instructions.at(caret))->type == rbc_instruction::PUSH)
                        {
                            parameters.push_back(*cmd->parameters.at(2));
                            mcprogram.varStackCount--;
                        }
                        std::vector<rbc_value> reversed;
                        reversed.reserve(parameters.size());

                        for (auto it = parameters.rbegin(); it != parameters.rend(); ++it) {
                            reversed.push_back(std::move(*it));
                        }
                        parameters = std::move(reversed);
                        auto decl = inb_impls::INB_IMPLS_MAP.find(name);
                        if (decl == inb_impls::INB_IMPLS_MAP.end())
                        {
                            err = "Fatal: inbuilt (__cpp__ decl) c++ function mapping for '" + name + "' doesn't exist. This could be due to a mismatch in versions.";
                            return {};
                        }
                        decl->second(program, factory, parameters, err);
                        if (!err.empty())
                            return {}; // todo can printerr here!!!
                    }
                    else
                    {
                        // we do need the parameters at runtime! the function is not inbuilt
                        factory.addBuffer();
                        factory.invoke(moduleName, func);
                        factory.clearBuffer();

                    }
                    while (i + 1 < instructions.size() && instructions.at(i + 1).type == rbc_instruction::POP)
                    {
                        i++;
                        factory.popParameter();
                        mcprogram.varStackCount--;
                    }

                    break;
                }
                case rbc_instruction::PUSH:
                {
                    RS_ASSERT_SIZE(size >= 2);

                    // store PUSH generated commands into a buffer so that if an inbuilt function is called,
                    // we can clear the buffer as the function is handled at compile time.
                    if (!factory.usingBuffer())
                    {
                        factory.createBuffer();
                        factory.enableBuffer();
                    }

                    rbc_constant funcName = std::get<0>(*instruction.parameters.at(0));
                    rbc_constant paramName = std::get<0>(*instruction.parameters.at(1));

                    std::unordered_map<std::string, std::shared_ptr<rbc_function>>::iterator func;

                    if (size == 4)
                    {
                        rs_module* fromModule = (rs_module*) std::get<std::shared_ptr<void>>(*instruction.parameters.at(3)).get();
                        
                        if(!fromModule)
                        {
                            err = "Function defined in module has caused seg fault. Flag this error on the github, it should not occur.";
                            break;
                        }
                        else
                            func = fromModule->functions.find(funcName.val);
                    }
                    else
                        func = program.functions.find(funcName.val);
                    // TODO: change to param index?
                    rs_variable* param = func->second->getParameterByName(paramName.val);
                    // TODO: add null checks here

                    factory.createVariable(*param, *instruction.parameters.at(2));
                    mcprogram.stack.push_back(param);

                    break;
                }
                case rbc_instruction::IF:
                case rbc_instruction::NIF:
                {
                _parseif:
                    RS_ASSERT_SIZE(size > 0);
                    const bool invertFlag = instruction.type == rbc_instruction::NIF;

                    if (size == 1)
                    {
                        // bool convertable if statement
                        rbc_value& param = *instruction.parameters.at(0);
                        switch(param.index())
                        {
                            case 0:
                            {
                                rbc_constant& _const = std::get<0>(param);
                                if (_const.val_type != token_type::INT_LITERAL)
                                {
                                    // todo move error to torbc
                                    err = std::format("Cannot convert typeid {} to boolean.", static_cast<int>(_const.val_type));
                                    return {};
                                }
                                const int value = std::stoi(_const.val);

                                if (value == 0)
                                {
                                    // skip instructions contained
                                    while(++i < instructions.size())
                                    {
                                        auto& inst = instructions.at(i);
                                        if (inst.type == rbc_instruction::ELSE)
                                            goto skip;
                                        if (inst.type == rbc_instruction::ENDIF)
                                            break;
                                    }

                                    i++; // skip ENDIF
                                }
                                else
                                {
                                skip:
                                    // remove next end if and parse as normal
                                    size_t c = i;
                                    while(++c < instructions.size())
                                    {
                                        auto& inst = instructions.at(c);
                                        if(inst.type == rbc_instruction::ENDIF)
                                        {
                                            instructions.erase(instructions.begin() + c);
                                            break;
                                        }
                                    }
                                }
                                break;
                            }
                            case 1:
                            {
                                rbc_register& reg = *std::get<1>(param);
                                std::shared_ptr<comparison_register> outReg = nullptr;
                                if (reg.operable)
                                {
                                    // makes reg if needed
                                    outReg = factory.compareNull(true, MC_OPERABLE_REG(INS_L(STR(reg.id))), !invertFlag);
                                }
                                else
                                {
                                    outReg = factory.compareNull(false, MC_NOPERABLE_REG(reg.id), !invertFlag);
                                    // see if contents is also not 0
                                }
                                mcprogram.blocks.push({0, outReg});
                                break;
                            }
                            case 2:
                            {
                                rs_variable& var = *std::get<2>(param);
                                std::shared_ptr<comparison_register> outReg = factory.compareNull(false, MC_VARIABLE_VALUE(var.comp_info.varIndex), !invertFlag);
                                
                                mcprogram.blocks.push({0, outReg});
                                break;
                            }
                            default:
                                ERROR("Cannot compare rbc_value of unimplemented typeid to null. If you see this error, flag an issue.");
                        }
                        break;
                    }
                    
                    RS_ASSERT_SIZE(size == 3);
                    rbc_value& lhs = *instruction.parameters.at(0);
                    rbc_constant& op = std::get<0>(*instruction.parameters.at(1));
                    bool eq = op.val == "==";
                    if (invertFlag) eq = !eq;

                    rbc_value& rhs = *instruction.parameters.at(2);

                    // commutative check, as no values are modified
                    std::shared_ptr<comparison_register> usedRegister = nullptr;
                    if (lhs.index() == rhs.index())
                    {
                        switch(lhs.index())
                        {
                            case 0:
                            {
                                // two constants.
                                WARN("Comparing two constants is not good practice.");
                                break;
                            }
                            case 1:
                            {
                                // two registers
                                rbc_register& reg  = *std::get<1>(lhs);
                                rbc_register& reg2 = *std::get<1>(rhs);

                                if ((reg.operable && !reg2.operable) || (!reg.operable && reg2.operable))
                                {
                                    // store result in storage and compare
                                    rbc_register& operable =  reg2.operable ? reg2 : reg;
                                    rbc_register& noperable = reg2.operable ? reg  : reg2; 
                                    factory.add( factory.getRegisterValue(noperable).storeResult(PADR(scoreboard) MC_TEMP_SCOREBOARD_STORAGE) );

                                    usedRegister = factory.compare("score", MC_OPERABLE_REG(INS_L(STR(operable.id))), eq, MC_TEMP_SCOREBOARD_STORAGE);
                                }
                                // both are either operable or not operable
                                else if (reg.operable)
                                    usedRegister = factory.compare("score", MC_OPERABLE_REG(INS_L(STR(reg.id))), eq, MC_OPERABLE_REG(INS_L(STR(reg2.id))));
                                else // TODO fix NOPERABLE_REG_GET: not raw, has extra commands at start
                                    usedRegister = factory.compare("data", MC_NOPERABLE_REG_GET(reg.id), eq, MC_NOPERABLE_REG_GET(reg2.id));
                                break;
                            }
                            case 2:
                            {
                                rs_variable& var  = *std::get<2>(lhs);
                                rs_variable& var2 = *std::get<2>(rhs);

                                usedRegister = factory.compare("data", RS_PROGRAM_STORAGE SEP MC_VARIABLE_VALUE(var.comp_info.varIndex), eq,
                                                        RS_PROGRAM_STORAGE SEP MC_VARIABLE_VALUE(var2.comp_info.varIndex));

                                break;
                            }
                            default:
                                WARN("Unimplemented comparison.");
                        }
                    }
                    else
                    {
                        {
                        result_pair<sharedt<rbc_register>, rbc_constant> res = 
                            commutativeVariantEquals<sharedt<rbc_register>, rbc_constant, rbc_value>(1, lhs, 0, rhs);
                        if (res)
                        {
                            rbc_register& reg =   *res.i1;
                            rbc_constant& con =   *res.i2;

                            if (reg.operable)
                            {
                                usedRegister = factory.compare("score", MC_OPERABLE_REG(INS_L(STR(reg.id))), eq, con.val, true);
                            }
                            else
                            {
                                factory.create_and_push(MC_DATA_CMD_ID, MC_TEMP_STORAGE_SET_CONST(con.val));
                                // TODO: fix to not have leading keywords!
                                usedRegister = factory.compare("data", MC_NOPERABLE_REG_GET(reg.id), eq, MC_TEMP_STORAGE);
                            }
                            goto _end;
                        }
                        }
                        {
                        result_pair<sharedt<rbc_register>, sharedt<rs_variable>> res = 
                            commutativeVariantEquals<sharedt<rbc_register>, sharedt<rs_variable>, rbc_value>(1, lhs, 2, rhs);

                        if (res)
                        {
                            rbc_register& reg = *res.i1;
                            rs_variable&  var = *res.i2;

                            if (reg.operable)
                                factory.getRegisterValue(reg).storeResult(PADR(storage) MC_TEMP_STORAGE, "int", 1);
                            else
                                factory.copyStorage(MC_TEMP_STORAGE, MC_NOPERABLE_REG_GET(reg.id));
                            usedRegister = factory.compare("data", MC_VARIABLE_VALUE(var.comp_info.varIndex), eq, MC_TEMP_STORAGE);
                            goto _end;
                        }
                        }
                        {
                        result_pair<sharedt<rs_variable>, rbc_constant> res = 
                            commutativeVariantEquals<sharedt<rs_variable>, rbc_constant, rbc_value>(2, lhs, 0, rhs);

                        if (res)
                        {
                            rs_variable&  var = *res.i1;
                            rbc_constant& con = *res.i2;
                            
                            usedRegister = factory.compare("data", MC_VARIABLE_VALUE(var.comp_info.varIndex), eq, con.val, true);
                            goto _end;
                        }
                        }
                    }
                _end:
                    mcprogram.blocks.push({0, usedRegister});
                    break;
                }
                case rbc_instruction::ELSE:
                {
                    // pop the if off the blocks.
                    auto& block = mcprogram.blocks.top();
                    
                    auto reg = block.second;
                    
                    mcprogram.blocks.pop();
                    mcprogram.blocks.push({1, reg});
                    // todo
                    break;
                }
                case rbc_instruction::ELIF:
                {   
                    auto& block = mcprogram.blocks.top();
                    
                    auto reg = block.second;
                    
                    mcprogram.blocks.pop();
                    mcprogram.blocks.push({2, reg});
                    goto _parseif;
                }
                case rbc_instruction::ENDIF:
                {
                    auto& block = mcprogram.blocks.top();
                    block.second->vacant = true;

                    mcprogram.blocks.pop();
                    
                    // we have an elif
                    if (mcprogram.blocks.size() > 0 && mcprogram.blocks.top().first == 2)
                        mcprogram.blocks.pop();
                    break;
                }
                case rbc_instruction::RET:
                {
                    // TODO
                    // return 1 if a return value is present, 0 if not.
                    if (size > 0)
                    {
                        rbc_value& val = *instruction.parameters.at(0);

                        switch(val.index())
                        {
                            case 0:
                            {
                                rbc_constant& _const = std::get<0>(val);
                                _const.quoteIfStr();
                                // store constant in return register, return 1.
                                factory.create_and_push(MC_DATA_CMD_ID, MC_DATA(modify storage, RS_PROGRAM_RETURN_REGISTER) PAD(set value) INS_L(_const.val));
                                
                                // store its type info
                                factory.create_and_push(MC_DATA_CMD_ID, MC_DATA(modify storage, RS_PROGRAM_RETURN_TYPE_REGISTER) PAD(set value) INS_L(STR(static_cast<int>(_const.val_type))));
                                break;
                            }
                            case 1:
                            {
                                rbc_register& reg = *std::get<1>(val);

                                factory.getRegisterValue(reg).storeResult(PADR(storage) RS_PROGRAM_STORAGE SEP RS_PROGRAM_RETURN_REGISTER);

                                if (reg.operable)
                                {
                                    // must be int
                                    factory.create_and_push(MC_DATA_CMD_ID, MC_DATA(modify storage, RS_PROGRAM_RETURN_TYPE_REGISTER) PAD(set value) INS_L(STR(static_cast<int>(token_type::INT_LITERAL))));
                                }
                                else
                                {
                                    // not implemented
                                    ERROR("Cannot save type info for return value in non-operable register. Not implemented. This will cause the type function to fail for some variables.");
                                }

                                break;   
                            }
                            case 2:
                            {
                                rs_variable& var = *std::get<2>(val);

                                factory.copyStorage(RS_PROGRAM_STORAGE SEP RS_PROGRAM_RETURN_REGISTER, MC_VARIABLE_VALUE_FULL(var.comp_info.varIndex));
                                factory.copyStorage(RS_PROGRAM_STORAGE SEP RS_PROGRAM_RETURN_TYPE_REGISTER, MC_VARIABLE_TYPE_FULL(var.comp_info.varIndex));
                                break;
                            }
                            default:
                                ERROR("Unimplemented return case for object, list, etc.");
                        }
                        factory.Return(true);
                    }
                    else
                        factory.Return(false);
                    break;
                }
                case rbc_instruction::SAVERET:
                {
                    RS_ASSERT_SIZE(size == 1);

                    rs_variable& var = *std::get<2>(*instruction.parameters.at(0));

                    factory.copyStorage(MC_VARIABLE_VALUE(var.comp_info.varIndex), RS_PROGRAM_RETURN_REGISTER);
                    factory.copyStorage(MC_VARIABLE_TYPE(var.comp_info.varIndex) , RS_PROGRAM_RETURN_TYPE_REGISTER);
                    break;
                }
                default:
                    WARN("Unimplemented RBC instruction found.");
                    break;
            }
        }
        mccmdlist list = factory.package();
        factory.clear();
        return list;
    };
    
    try{
        mcprogram.globalFunction.commands = parseFunction(program.globalFunction.instructions);

        std::vector<std::shared_ptr<rbc_function>> allFunctions;


        // this code is a monstrosity, but all it does it get all the functions ever created and 
        // put them in 1 neat list.
        for(auto& func : program.functions)
        {
            allFunctions.push_back(func.second);
            for(auto& child : func.second->childFunctions)
                allFunctions.push_back(child.second);
        }
        // add module functions
        if (program.modules.size() > 0)
        {
            std::stack<std::shared_ptr<rs_module>> modules;
            
            for(auto& mod : program.modules)
                modules.push(mod.second);

            // do search to get all module functions
            while (!modules.empty())
            {
                auto& mod = modules.top();
                auto& newFunctions = mod->functions;
                if (newFunctions.size() > 0)
                {
                    for(auto& func : newFunctions)
                    {
                        allFunctions.push_back(func.second);

                        for(auto& child : func.second->childFunctions)
                            allFunctions.push_back(child.second);
                    }

                }

                for(auto& child : mod->children)
                    modules.push(child.second);

                modules.pop();
            }
        }

        for(auto& function : allFunctions)
        {
            auto& decorators = function->decorators;
            if 
            (
                std::find(decorators.begin(), decorators.end(), rbc_function_decorator::CPP) == decorators.end() &&
                std::find(decorators.begin(), decorators.end(), rbc_function_decorator::EXTERN) == decorators.end()
            ) // not inbuilt function 
            {
                // todo clean up
                if (function->generics)
                {
                    rbc_function_generics& generics = *function->generics;

                    for (auto& entry : generics.variations)
                    {
                        mc_function f{entry.second->name,
                              parseFunction(entry.second->instructions),
                              entry.second->modulePath};
                        f.parentalHashStr = entry.second->getParentHashStr();
                        f.genericHashStr  = entry.second->getGenericsHashStr();

                        mcprogram.functions.push_back(f);
                    }
                }
                else
                {
                    mc_function f{function->name,
                                parseFunction(function->instructions),
                                function->modulePath};
                    f.parentalHashStr = function->getParentHashStr();
                    mcprogram.functions.push_back(f);
                }
            }
        }
    } catch (std::exception& e)
    {
        err = std::string("Internal error: ") + e.what();
        return mcprogram;
    }
    factory.initProgram();
    mccmdlist init = factory.package();
    mcprogram.globalFunction.commands.insert(mcprogram.globalFunction.commands.begin(), init.begin(), init.end());

    return mcprogram;
}
namespace conversion
{
    void                  CommandFactory::initProgram      ()
    {
        // ROOT
        _nonConditionalFlag = true;
        create_and_push(MC_DATA_CMD_ID, MC_DATA(merge storage, RS_PROGRAM_DATA_DEFAULT));

        // OPERABLE REGISTERS
        // todo fix. min reg count isnt calculated correctly.
        create_and_push(MC_SCOREBOARD_CMD_ID, "objectives add temp dummy \"temp\"");

        mccmdlist programInit;

        for(size_t i = 0; i < context.comparisonRegisters.size(); i++)
            programInit.push_back(mc_command{false, MC_SCOREBOARD_CMD_ID, MC_CREATE_COMPARISON_REGISTER(i, "dummy")});
        for(size_t i = 0; i < rbc_compiler.registers.size(); i++)
            programInit.push_back(mc_command{false, MC_SCOREBOARD_CMD_ID, MC_CREATE_OPERABLE_REG(i, "dummy")});

        commands.insert(commands.begin(), programInit.begin(), programInit.end());

        _nonConditionalFlag = false;
    }
    CommandFactory::_This CommandFactory::Return           (bool val)
    {
        create_and_push(MC_RETURN_CMD_ID, val ? "1" : "0");
        return THIS;
    }

    CommandFactory::_This CommandFactory::pushParameter    (rbc_value& val)
    {
        switch(val.index())
        {
            case 0:
            {
                rbc_constant& c = std::get<rbc_constant>(val);
                c.quoteIfStr();
                create_and_push(MC_DATA_CMD_ID, MC_STACK_PUSH_CONST(c.val));
                break;
            }
            case 1:
            {
                rbc_register& reg = *std::get<sharedt<rbc_register>>(val);
                add(getRegisterValue(reg).storeResult(
                    PADR(storage) RS_PROGRAM_DATA SEP RS_PROGRAM_STACK
                ));
                break;
            }
            case 2:
            {
                rs_variable& var = *std::get<sharedt<rs_variable>>(val);
                appendStorage(RS_PROGRAM_STORAGE SEP RS_PROGRAM_STACK, MC_VARIABLE_VALUE(var.comp_info.varIndex));
                break;
            }
        }
        return THIS;
    }
    CommandFactory::_This CommandFactory::invoke           (const std::string& module, rbc_function& func)
    {
        // TODO: MACROS & NAMESPACES

        std::string parentHashStr = func.getParentHashStr();
        if (!parentHashStr.empty()) parentHashStr.push_back('_');

        std::string name = parentHashStr + func.name;

        if (func.assignedGenerics)
            name += "_g_" + func.getGenericsHashStr();

        if (func.modulePath.empty())
            create_and_push(MC_FUNCTION_CMD_ID, module + ':' + name);
        else
        {
            std::string path;
            for(std::string& s : func.modulePath)
                path += s + '/';

            create_and_push(MC_FUNCTION_CMD_ID, module + ':' + path + name);
        }
        return THIS;
    }
    CommandFactory::_This CommandFactory::popParameter     ()
    {
        rs_variable* var = context.stack.back();
        create_and_push(MC_DATA_CMD_ID, MC_DATA(remove storage, ARR_AT(RS_PROGRAM_VARIABLES, STR(var->comp_info.varIndex))));
        context.stack.pop_back();
        return THIS;
    }
    mc_command            CommandFactory::getRegisterValue (rbc_register& reg)
    {
        if (reg.operable)
            return mc_command(false, MC_SCOREBOARD_CMD_ID, MC_OPERABLE_REG_GET(reg.id));
        return mc_command(false, MC_DATA_CMD_ID, MC_NOPERABLE_REG_GET(reg.id));
    }
    mc_command            CommandFactory::getStackValue    (long index)
    {
        return mc_command(false, MC_DATA_CMD_ID, MC_GET_STACK_VALUE(index));
    }
    CommandFactory::_This CommandFactory::setVariableValue (rs_variable& var, rbc_value& val)
    {
        switch(val.index())
        {
            // constant
            case 0:
            {
                rbc_constant& c = std::get<0>(val);
                c.quoteIfStr();
                create_and_push(MC_DATA_CMD_ID, MC_VARIABLE_SET_CONST(var.comp_info.varIndex, c.val));
                break;
            }
            default:
                ERROR("Unsupported SAVE operation. TODO implement!");
        }
        return THIS;
    }
    CommandFactory::_This CommandFactory::setRegisterValue (rbc_register& reg, rbc_value& value)
    {
        switch(value.index())
        {
            case 0:
            {
                rbc_constant& val = std::get<rbc_constant>(value);
                val.quoteIfStr();
                if (reg.operable)
                    create_and_push(MC_SCOREBOARD_CMD_ID, MC_OPERABLE_REG_SET(reg.id, val.val));
                else
                    create_and_push(MC_DATA_CMD_ID, MC_DATA(modify storage, ARR_AT(RS_PROGRAM_REGISTERS, STR(reg.id)))
                                                            PAD(set value) INS_L(val.val));
                break;
            }
            case 1:
            {
                // TODO
                break;
            }
            case 2:
            {
                rs_variable& var = *std::get<2>(value);
                if (reg.operable)
                {
                    mc_command cmd = CommandFactory::getVariableValue(var).storeResult(
                        PADR(score) MC_OPERABLE_REG(INS_L(STR(reg.id)))
                    );
                    
                    add(cmd);
                }
                else
                    ERROR("Unsupported! TODO FIX");
            }
        }
        return THIS;
    }
    mc_command            CommandFactory::getVariableValue (rs_variable& var)
    {
        return mc_command(false, MC_DATA_CMD_ID, MC_GET_VARIABLE_VALUE(var.comp_info.varIndex));
    }
    std::shared_ptr<comparison_register> CommandFactory::compareNull   (const bool scoreboard, const std::string& where, const bool eq)
    {
        auto destreg = getFreeComparisonRegister();
        destreg->vacant = false;
     
        destreg->operation = eq ? comparison_operation_type::EQ : comparison_operation_type::NEQ;
        
     
        if (scoreboard)
        {
            mc_command cmd(false, MC_SCOREBOARD_CMD_ID, MC_COMPARE_REG_SET(destreg->id, "1"));
            cmd.ifint(where, destreg->operation, "0", true, eq);

            add(cmd);
        }
        else
        {
            // if we can merge the contents of the variable into temp, then it is not 0, and not null.
            create_and_push(MC_SCOREBOARD_CMD_ID, MC_TEMP_STORAGE_SCOREBOARD_SET_RAW_CONST("0"));
            add(makeCopyStorage(MC_TEMP_STORAGE_NAME, where).storeSuccess(PADR(score) MC_COMPARE_REG_FULL(destreg->id)));
        }

        return destreg;
    }
    std::shared_ptr<comparison_register> CommandFactory::getFreeComparisonRegister()
    {
        std::shared_ptr<comparison_register> reg = context.getFreeComparisonRegister();

        if (!reg)
        {
            comparison_register _new;
            size_t id = context.comparisonRegisters.size();

            _new.id = id;
            context.comparisonRegisters.push_back(std::make_shared<comparison_register>(_new));

            reg = context.comparisonRegisters[id];
        }
        return reg;
    }
    std::shared_ptr<comparison_register> CommandFactory::compare          (const std::string& locationType,
                                                            const std::string& lhs,
                                                            const bool eq,
                                                            const std::string& rhs,
                                                            const bool rhsIsConstant)
    {
        std::shared_ptr<comparison_register> reg = getFreeComparisonRegister();

        reg->vacant = false;

        if (locationType == "data")
        {


            // copy storage value to storage temp
            // try copy rhs to storage temp, and store success in next comparison register
            // invert operation
            reg->operation = eq ? comparison_operation_type::NEQ : comparison_operation_type::EQ;
            if (rhsIsConstant)
            {
                create_and_push(MC_DATA_CMD_ID, MC_TEMP_STORAGE_SET_CONST(rhs));
                mc_command cmd = makeCopyStorage(MC_TEMP_STORAGE_NAME, lhs).storeSuccess(PADR(score) MC_COMPARE_REG_FULL(reg->id));
                add(cmd);
            }
            else
            {
                mc_command cmd = makeCopyStorage(MC_TEMP_STORAGE_NAME, rhs).storeSuccess(PADR(score) MC_COMPARE_REG_FULL(reg->id));
                copyStorage(MC_TEMP_STORAGE_NAME, lhs);
                add(cmd);

            }

        }
        else if (locationType == "score")
        {
            create_and_push(MC_SCOREBOARD_CMD_ID, MC_COMPARE_RESET(reg->id));
            reg->operation = eq ? comparison_operation_type::EQ : comparison_operation_type::NEQ;
            mc_command m{false, MC_SCOREBOARD_CMD_ID, PADR(players set) MC_COMPARE_REG_GET_RAW(INS(STR(reg->id))) PADL(1)};

            m.ifint(lhs, reg->operation, rhs, rhsIsConstant, !eq);

            add(m);
        }
        else
            WARN("Unknown comparison operation flag.");

        return reg;

    }

    void                  CommandFactory::make             (mc_command& cmd)
    {
        if (context.blocks.size() == 0 || _nonConditionalFlag)
            return;
        auto iter = context.blocks.end();
        while (iter != context.blocks.begin())
        {
            iter--;
            auto& block = *iter;
            auto& blockRegister = *block.second;
            switch(block.first)
            {
                case 0:
                    // used to use comparison_operation_type::EQ
                    cmd.ifcmpreg(blockRegister.operation, blockRegister.id);
                    break;
                case 1:
                case 2:
                {
                    comparison_operation_type opposite = blockRegister.operation == comparison_operation_type::EQ ? comparison_operation_type::NEQ : comparison_operation_type::EQ;
                    cmd.ifcmpreg(opposite, blockRegister.id);
                    break;
                }
                default: // TODO: add elif
                    break;
            }
        }


        cmd.cmd = MC_EXEC_CMD_ID;
        
        // etc...

    }
    CommandFactory::_This CommandFactory::appendStorage    (const std::string& dest, const std::string& _const)
    {
        create_and_push(MC_DATA_CMD_ID, MC_DATA(modify storage, INS(dest)) PAD(append value) INS_L(_const));
        return THIS;
    }
    mc_command CommandFactory::makeAppendStorage    (const std::string& dest, const std::string& _const)
    {
        return mc_command(false, MC_DATA_CMD_ID, MC_DATA(modify storage, INS(dest)) PAD(append value) INS_L(_const));
    }
    CommandFactory::_This CommandFactory::copyStorage      (const std::string& dest, const std::string& src)
    {
        create_and_push(MC_DATA_CMD_ID, MC_DATA(modify storage, INS(dest))
                                            SEP
                                        MC_DATA(set from storage, INS_L(src)));
        return THIS;
    }
    mc_command CommandFactory::makeCopyStorage              (const std::string& dest, const std::string& src)
    {
        return mc_command(false, MC_DATA_CMD_ID, MC_DATA(modify storage, INS(dest))
                                            SEP
                                        MC_DATA(set from storage, INS_L(src)));
    }
    CommandFactory::_This CommandFactory::createVariable   (rs_variable& var)
    {
        create_and_push(MC_DATA_CMD_ID,
                MC_DATA(modify storage, RS_PROGRAM_VARIABLES)
                    PAD(append value)
                MC_VARIABLE_JSON_DEFAULT(std::to_string(var.scope),
                                        std::to_string(var.type_info.type_id))
                        );
        var.comp_info.varIndex = context.varStackCount++;
        return THIS;
    }
    CommandFactory::_This CommandFactory::createVariable   (rs_variable& var, rbc_value& val)
    {
        switch(val.index())
        {
            case 0:
            {
                var.comp_info.varIndex = context.varStackCount++;

                rbc_constant& c = std::get<0>(val);
                c.quoteIfStr();
                create_and_push(MC_DATA_CMD_ID,
                    MC_DATA(modify storage, RS_PROGRAM_VARIABLES)
                        PAD(append value)
                    MC_VARIABLE_JSON_VAL(c.val, std::to_string(var.scope),
                                                std::to_string(var.type_info.type_id))
                                );
                break;
            }
            case 1:
            {
                // var.comp_info.varIndex = context.varStackCount++;
                // handled in create variable
                sharedt<rbc_register>& reg = std::get<1>(val);
                createVariable(var);
                add( getRegisterValue(*reg).storeResult(PADR(storage) MC_VARIABLE_VALUE_FULL(var.comp_info.varIndex), "int", 1) );
                
                break;
            }
            case 2:
            {
                // handled in create variable
                sharedt<rs_variable>& variable = std::get<2>(val);
                createVariable(var);
                copyStorage(MC_VARIABLE_VALUE(var.comp_info.varIndex), MC_VARIABLE_VALUE(variable->comp_info.varIndex));
                break;
            }
            case 3:
            {
                // TODO
                break;
            }
            case 4:
            {
                //  0, 1, 2, 3
                // v = [4, x, 4, y]

                rs_list& l = *std::get<4>(val);

                std::stringstream listInitStr;
                listInitStr << '[';

                std::vector<uint32_t> uninitialized;
                std::vector<mc_command> initCommands;
                for(auto& value : l.values)
                {
                    switch(value->index())
                    {
                        case 0:
                        {
                            rbc_constant& c = std::get<0>(*value);
                            c.quoteIfStr();

                            listInitStr << c.val << ',';

                            break;
                        }
                        case 1:
                        {
                            rbc_register& reg = *std::get<1>(*value);
                            
                            if(reg.operable)
                            {
                                mc_command assign(false, MC_DATA_CMD_ID, MC_DATA(modify storage, MC_VARIABLE_VALUE(var.comp_info.varIndex))
                                                        PAD(append from score) MC_OPERABLE_REG(INS_L(STR(reg.id)))
                                                );
                                initCommands.push_back(assign);

                            }
                            else
                            {
                                // TODO FIX
                                mc_command assign(false, MC_DATA_CMD_ID, MC_DATA(modify storage, MC_VARIABLE_VALUE(var.comp_info.varIndex))
                                                        PAD(append from storage) MC_NOPERABLE_REG(reg.id)
                                            );
                                initCommands.push_back(assign);
                            }
                            break;
                        }
                        case 2:
                        {
                            // insert 0, and append (move code to below).
                            rs_variable& v = *std::get<2>(*value);

                            mc_command assign(false, MC_DATA_CMD_ID, MC_DATA(modify storage, MC_VARIABLE_VALUE(var.comp_info.varIndex))
                                                        PAD(append from storage) MC_VARIABLE_VALUE_FULL(v.comp_info.varIndex)
                                            );
                            initCommands.push_back(assign);
                            break;
                        }
                    }
                }
                listInitStr.seekp(-1, std::ios_base::end);
                listInitStr << ']';

                create_and_push(MC_DATA_CMD_ID,
                    MC_DATA(modify storage, RS_PROGRAM_VARIABLES)
                        PAD(append value)
                    MC_VARIABLE_JSON_VAL(listInitStr.str(), std::to_string(var.scope),
                                                std::to_string(var.type_info.type_id))
                                );

                // add all commands after we init
                for (auto& cmd : initCommands) add(cmd);

                break;
            }
        }
        return THIS;
    }
    CommandFactory::_This CommandFactory::op_reg_math      (rbc_register& reg, rbc_value& val, bst_operation_type t)
    {
        switch(val.index())
        {
            case 0:
            {
                rbc_constant& c = std::get<0>(val);
                c.quoteIfStr();
                // we can add/subtract constants easily using scoreboard add/remove.
                // with other operations however, we cant, and need to store this constant in the next free register.
                
                if (t == bst_operation_type::ADD)
                {
                    create_and_push(MC_SCOREBOARD_CMD_ID, MC_REG_INCREMENT_CONST(reg.id, c.val));
                    return THIS;
                }
                else if (t == bst_operation_type::SUB)
                {
                    create_and_push(MC_SCOREBOARD_CMD_ID, MC_REG_DECREMENT_CONST(reg.id, c.val));
                    return THIS;
                }
                sharedt<rbc_register> rhReg = rbc_compiler.getFreeRegister(true);
                if (!rhReg)
                    rhReg = rbc_compiler.makeRegister(true);
                setRegisterValue(*rhReg, val);
                const std::string opStr = operationTypeToStr(t) + '=';
                switch(t)
                {
                    case bst_operation_type::MUL:
                    case bst_operation_type::DIV:
                    case bst_operation_type::MOD:
                    case bst_operation_type::XOR:
                    {
                        create_and_push(MC_SCOREBOARD_CMD_ID, MC_REG_OPERATE(reg.id, opStr, rhReg->id));
                        break;
                    }
                    default:
                        ERROR("Unknown/Unsupported math operation between register and constant.");
                }
                break;
            }
            case 1:
            {
                break;
            }
            case 2:
            {
                break;
            }
            default:
                ERROR("Register cannot store unsupported value.");
                return THIS;
        }
        return THIS;
    }
    CommandFactory::_This CommandFactory::math             (rbc_value& lhs, rbc_value& rhs, bst_operation_type op)
    {

        switch(lhs.index())
        {
            case 1:
            {
                rbc_register& reg = *std::get<1>(lhs);

                reg.operable ? op_reg_math(reg, rhs, op) : nop_reg_math(reg, rhs, op);
                reg.free();
                break;
            }
            case 2:
            {
                switch(rhs.index())
                {
                    case 1:
                    {
                        rbc_register& reg  = *std::get<1>(rhs);
                        reg.operable ? op_reg_math(reg, lhs, op) : nop_reg_math(reg, lhs, op);
                        reg.free();
                        break;
                    }
                    default:
                        ERROR("Constant operation is unsupported here.");
                        return THIS;
                }
                break;
            }
            default:
                ERROR("Constant operation is unsupported here.");
                return THIS;
        }
        return THIS;
    }
}


#undef COMP_ERROR
#undef COMP_ERROR_R