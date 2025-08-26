#pragma once
#include <unordered_map>
#include <vector>
#include <variant>
#include <memory>

#include "globals.hpp"
#include "types/rbc_value.hpp"
#include "type_info.hpp"
#include "mc.hpp"

// forward decls for rbc_value
namespace conversion
{
    class CommandFactory;
};
struct rbc_program;
#define INB_IMPL_PARAMETERS [[maybe_unused]] rbc_program& program,                \
                            [[maybe_unused]] conversion::CommandFactory& factory, \
                            [[maybe_unused]] std::vector<rbc_value>& parameters,  \
                            [[maybe_unused]] std::vector<rs_type_info>* generics, \
                            [[maybe_unused]] std::string& err 

struct function_locator
{
    std::vector<std::string> path;

    function_locator(std::vector<std::string> v) : path(v) {}
    function_locator(const std::string& s) : path({s}) {}
    function_locator(const char* ch) : path({std::string(ch)}) {}
    function_locator(std::initializer_list<std::string> ilist) : path(ilist) {}

    function_locator(){}

    // Assignment from std::vector<std::string>
    function_locator& operator=(const std::vector<std::string>& v) {
        path = v;
        return *this;
    }

    // Assignment from std::string
    function_locator& operator=(const std::string& s) {
        path = {s};
        return *this;
    }

    // Assignment from const char*
    function_locator& operator=(const char* s) {
        path = {std::string(s)};
        return *this;
    }

    // Assignment from initializer list
    function_locator& operator=(std::initializer_list<std::string> ilist) {
        path = ilist;
        return *this;
    }

    // Equality operator
    bool operator==(const function_locator& other) const {
        return path == other.path;
    }
    std::string str() const
    {
        std::string s;
        const size_t size = path.size();
        for(size_t i = 0; i < size; i++)
        {
            s += path.at(i);

            if (i != size - 1)
                s += "::";
        }
        return s;
    }

};

inline void hash_combine(std::size_t& seed, std::size_t value) {
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// Specialize std::hash
namespace std {
    template <>
    struct hash<function_locator> {
        std::size_t operator()(const function_locator& loc) const {
            std::size_t seed = 0;
            for (const auto& s : loc.path) {
                hash_combine(seed, std::hash<std::string>{}(s));
            }
            return seed;
        }
    };
}

using _InbRetV = std::variant<rbc_value, mc_command>;
using _InbRetT = std::shared_ptr<_InbRetV>;

namespace inb_impls
{
    _InbRetT msg(INB_IMPL_PARAMETERS);
    _InbRetT kill(INB_IMPL_PARAMETERS);

    namespace debug
    {
        _InbRetT print(INB_IMPL_PARAMETERS);
    };
    namespace time
    {
        _InbRetT now(INB_IMPL_PARAMETERS);
    }

    inline std::unordered_map<function_locator, _InbRetT(*)(INB_IMPL_PARAMETERS)> INB_IMPLS_MAP = 
    {
        {"msg", msg},
        {"kill", kill},



        /* DEBUG LIB */
        {{"debug", "print"}, debug::print},
        {{"time",  "now"  }, time::now}
    };
}