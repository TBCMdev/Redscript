#pragma once
#include <vector>
#include <functional>
#include <any>
#include <memory>
#include <unordered_map>
#include <map>
#include <stack>
#include <queue>
#include <filesystem>
#include <cstdint>
#include <deque>

#include "globals.hpp"
#include "mc.hpp"
#include "token.hpp"
#include "error.hpp"
#include "util.hpp"
#include "inb.hpp"

#include "type_info.hpp"
#include "types.hpp"



rbc_function_decorator parseDecorator(const std::string &);

struct rbc_command
{
    rbc_instruction type;
    std::vector<std::shared_ptr<rbc_value>> parameters;

    template <typename... _RBCValues>
    rbc_command(rbc_instruction _type, _RBCValues &&...values)
        : type(_type)
    {
        (parameters.emplace_back(std::make_shared<rbc_value>(std::forward<_RBCValues>(values))), ...);
    }
    rbc_command(rbc_instruction _type) : type(_type) {}

    // defined outside due to forward decl
    std::string tostr();
    std::string toHumanStr();
};



struct rbc_function_generics
{
    std::unordered_map<std::vector<rs_type_info>, std::shared_ptr<rbc_function>> variations;
    std::unordered_map<std::string, bool> entries;

    std::shared_ptr<project_fragment> fromFragment;
    // used for locating the function body in the fromFragment (parsing)
    size_t begin, end;
};

struct raw_rbc_function
{
    std::unordered_map<std::string, rbc_func_var_t> localVariables;
    std::vector<rbc_command> instructions;
};
struct rbc_function
{
    std::string name;
    int scope = 0;
    std::unordered_map<std::string, rbc_func_var_t> localVariables;
    std::vector<std::shared_ptr<rs_variable>> parameters;
    std::shared_ptr<rbc_function_generics> generics;
    std::shared_ptr<std::vector<rs_type_info>> assignedGenerics = nullptr;
    std::vector<rbc_command> instructions;
    std::vector<rbc_function_decorator> decorators;
    // made shared because of forward declaration
    std::shared_ptr<rs_type_info> returnType;
    std::vector<std::string> modulePath;
    std::shared_ptr<rbc_function> parent = nullptr;
    std::unordered_map<std::string, std::shared_ptr<rbc_function>> childFunctions;

    bool hasBody = true;

    rs_variable *getNthParameter(size_t p);
    rs_variable *getParameterByName(const std::string &name);
    std::string getParentHashStr();
    std::string getGenericsHashStr();
    std::string toStr();
    std::string toSignatureStr();
    std::string toHumanStr();
    rbc_function(const std::string& _name) : name(_name)
    {} 
    rbc_function(const rbc_function& other, const std::vector<rs_type_info>& resolvedGenerics);

};
struct rs_module
{
    // only supports functions for now
    std::string name;
    std::vector<std::string> modulePath;
    std::unordered_map<std::string, std::shared_ptr<rbc_function>> functions;
    std::unordered_map<std::string, std::shared_ptr<rs_module>> children;
};
struct rbc_program
{
    rs_error *context;
    int32_t currentScope = 0;
    std::stack<rbc_scope_type> scopeStack;
    raw_rbc_function globalFunction;

    std::shared_ptr<project_fragment> currentFragment = nullptr;

    rbc_scope_type lastScope;
    std::shared_ptr<rs_module> currentModule = nullptr;
    std::shared_ptr<rbc_function> currentFunction = nullptr;
    iterable_stack<std::shared_ptr<rbc_function>> functionStack;
    bool                                          _debug_infuncbody = false;
    iterable_stack<std::shared_ptr<rs_module>> moduleStack;
    std::vector<std::shared_ptr<rs_variable>> globalVariables;
    std::vector<std::shared_ptr<rbc_register>> registers;
    std::unordered_map<std::string, std::shared_ptr<rs_object>> objectTypes;
    std::unordered_map<std::string, std::shared_ptr<rs_module>> modules;
    std::unordered_map<std::string, std::shared_ptr<rbc_function>> functions;
    //                                    index, isObjectType
    std::unordered_map<std::string, std::pair<size_t, bool>> currentGenericTypes;
    std::vector<rs_type_info> genericTypeConversions;

public:
    sharedt<rs_variable>  getVariable(const std::string &name);
    sharedt<rbc_register> getFreeRegister(bool operable = false);
    sharedt<rbc_register> makeRegister(bool operable = false, bool vacant = true);

    std::vector<std::string> callStackStr();

    void operator()(std::vector<rbc_command> &instructions);
    void operator()(const rbc_command &instruction);
    template <typename First, typename Second, typename... _Command>
    inline void operator()(First &&f, Second &&s, _Command &&...commands)
    {
        operator()(f);
        operator()(s);
        (operator()(commands), ...);
    }

    rbc_program(rs_error *_context)
        : context(_context), currentScope(0), globalFunction{}, lastScope{}, currentModule(nullptr), currentFunction(nullptr)
    {
        // Other containers default-initialize themselves
    }
};

namespace rbc_commands
{
    namespace registers
    {
        rbc_command occupy(std::shared_ptr<rbc_register> reg, rbc_value val);
        // needs register copies to not seg fault when converting to variant.
        rbc_command operate(std::shared_ptr<rbc_register> reg, rbc_value val, uint op);
    };
    namespace variables
    {
        rbc_command create(std::shared_ptr<rs_variable> v, rbc_value val);
        rbc_command create(std::shared_ptr<rs_variable> v);
        rbc_command storeReturn(std::shared_ptr<rs_variable> v);
        rbc_command set(std::shared_ptr<rs_variable> v, rbc_value val);
    };
};

void preprocess(token_list &, std::string, std::string &, rs_error *, fragment_ptr_deque &,
                std::shared_ptr<std::vector<std::filesystem::path>> = nullptr);

namespace conversion
{
    class CommandFactory
    {
    public:
        using _This = CommandFactory &;

    private:
        bool _nonConditionalFlag = false;

        bool _useBuffer = false;

        mccmdlist commands;
        mc_program &context;
        rbc_program &rbc_compiler;

        _This op_reg_math(rbc_register &reg, rbc_value &val, bst_operation_type t);
        inline _This nop_reg_math(rbc_register &, rbc_value &, bst_operation_type)
        {
            WARN("Non operable register math is not supported.");
            return THIS;
        }

    public:
        std::shared_ptr<mccmdlist> _buffer;

        CommandFactory(mc_program &_context, rbc_program &_rbc_compiler) : context(_context), rbc_compiler(_rbc_compiler)
        {
        }
        inline void add(mc_command &c)
        {
            make(c);
            if (_useBuffer)
                _buffer->push_back(c);
            else
                commands.push_back(c);
        }
        template <typename... Tys>
        inline void create_and_push(Tys &&...t)
        {
            mc_command c(false, std::forward<Tys>(t)...);
            add(c);
        }
        inline mccmdlist package()
        {
            for (auto &cmd : commands)
                cmd.addroot();
            return commands;
        }
        inline void clear()
        {
            commands.clear();
        }
        inline void pop_back()
        {
            commands.pop_back();
        }
#pragma region buffer
        inline void createBuffer()
        {
            if (!_buffer)
                _buffer = std::make_shared<mccmdlist>();
        }
        inline void enableBuffer()
        {
            _useBuffer = true;
        }
        inline bool usingBuffer()
        {
            return _useBuffer;
        }
        inline void disableBuffer()
        {
            _useBuffer = false;
        }
        inline void clearBuffer()
        {
            if (_buffer)
                _buffer->clear();
        }
        inline void addBuffer()
        {
            if (!_buffer)
                return;

            commands.insert(commands.end(), _buffer->begin(), _buffer->end());
        }
#pragma endregion buffer
        void make(mc_command &in);

        void initProgram();

        _This copyStorage(const std::string &dest, const std::string &src);
        _This appendStorage(const std::string &dest, const std::string &_const);

        _This createVariable(rs_variable &var);
        _This createVariable(rs_variable &var, rbc_value &val);
        _This math(rbc_value &lhs, rbc_value &rhs, bst_operation_type t);
        _This pushParameter(rbc_value &val);
        _This popParameter();
        _This invoke(const std::string &module, rbc_function &func);
        _This Return(bool val);
        std::shared_ptr<comparison_register> compareNull(const bool scoreboard, const std::string &where, const bool eq);
        std::shared_ptr<comparison_register> compare(const std::string &locationType, const std::string &lhs,
                                                     const bool eq,
                                                     const std::string &rhs,
                                                     const bool rhsIsConstant = false);

        std::shared_ptr<comparison_register> getFreeComparisonRegister();
        static mc_command makeCopyStorage(const std::string &dest, const std::string &src);
        static mc_command getVariableValue(rs_variable &var);
        static mc_command getRegisterValue(rbc_register &reg);
        static mc_command makeAppendStorage(const std::string &dest, const std::string &_const);

        static mc_command            getStackValue(long index);
        constexpr static std::string getTypedNullConstant(const rs_type_info& t);
        static std::string           accessList(const std::vector<size_t>& indicies);

        _This setRegisterValue(rbc_register &reg, rbc_value &c);
        _This setVariableValue(rs_variable &var, rbc_value &val);
    };
}

mc_program tomc(rbc_program &, const std::string &, std::string &);
