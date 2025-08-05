#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <functional>
#include <format>

#include "constants.hpp"
#include "globals.hpp"

struct rs_type_info
{
    
    int32_t type_id     = -1; // first type id
    uint32_t array_count = 0;

    bool optional = false;
    bool reference   = false;

    bool    generic    = false;
    int32_t generic_id = -1;

    std::vector<rs_type_info> otherTypes = {}; // others if specified
    //              arrOptional, arrStrict

    std::vector<std::pair<bool, bool>> arrayFlags = {};
    std::string find_generic_type_name() const;
    inline std::string full_type_name() const
    {
        std::string ret = type_name();
        if (optional)
            ret.push_back('?');
        if (reference)
            ret.push_back('&');

        for(uint32_t i = 0; i < array_count; i++)
        {
            ret += "[]";
            auto& flag = arrayFlags.at(i);
            if (flag.first)
                ret.push_back('?');
            if (flag.second)
                ret.push_back('&');
        }
        return ret;
    }
    inline std::string type_name() const
    {
        if (generic)
            return find_generic_type_name();
        switch(type_id)
        {
            case RS_INT_KW_ID:
                return "int";
            case RS_STRING_KW_ID:
                return "string";
            case RS_FLOAT_KW_ID:
                return "float";
            case RS_BOOL_KW_ID:
                return "bool";
            case RS_OBJECT_KW_ID:
                return "object";
            default:
                return "unknown";
        }
    }
    inline static std::string type_name(int32_t id)
    {
        return rs_type_info{id, 0}.type_name();
    }
    inline std::string tostr() const
    {
        std::string typestr = full_type_name();
        
        for(size_t i = 0; i < otherTypes.size(); i++)
            typestr += '|' + otherTypes.at(i).tostr();

        // if (array_count != 0)
        //     typestr += '[' + std::to_string(array_count) + ']';  
            
        
        return typestr;
    }
    inline rs_type_info parent_type(bool op = false, bool ref = false) const
    {
        auto flagsCopy = arrayFlags;
        flagsCopy.push_back({op, ref});

        return rs_type_info{type_id, array_count + 1, optional, reference, generic, generic_id, otherTypes, flagsCopy};
    }
    inline rs_type_info element_type() const
    {
        if (array_count == 0) return *this;
        if (array_count == 1) return rs_type_info{type_id, 0, optional, reference, generic, generic_id, otherTypes, {}};

        // if (flagsCopy.size() > 0)
            // flagsCopy.pop_back();
        return rs_type_info{type_id, array_count - 1, optional, reference, generic, generic_id, otherTypes, arrayFlags};
    }
    inline bool compareArrayFlags(const rs_type_info& other) const
    {
        if (array_count != other.array_count) return false;

        for (size_t i = 0; i < array_count; i++)
        {
            const std::pair<bool, bool>& item = arrayFlags.at(i);
            const std::pair<bool, bool>& otherItem = other.arrayFlags.at(i);

            if (!typeDecoratorsEqualOrConvertable(item, otherItem))
                return false;
        }

        return true;
    }
    // when a type such as T[]? exists, the ? infers that the type is optional.
    // however, in this struct, optional = false, as the optional field applies solely
    // to the type T, so T?[] would yield optional = true.
    // this function returns whether the entire array type is optional.
    inline bool isFinallyOptional() const
    {
        if (array_count > 0)
            return arrayFlags.back().first;
        return optional;
    }
    inline bool equals(const rs_type_info& other) const
    {
        const bool aeq = array_count == other.array_count && compareArrayFlags(other);
        // TODO: needs redo for when references are implemented
        const bool finallyOptional      = isFinallyOptional();
        const bool otherFinallyOptional = other.isFinallyOptional();

        const bool meq = (!finallyOptional && !otherFinallyOptional) || (finallyOptional && !other.reference);

        return (((generic && !other.generic) && meq)
            ||  (generic && generic_id == other.generic_id && meq))
            || (other.type_id == RS_NULL_KW_ID && ((optional && array_count == 0) || finallyOptional)) // for null comparisons
            || (other.type_id == type_id && aeq && other.optional == optional && other.reference == reference)
            || canConvertTo(other);
    }
    inline bool equals(int32_t type) const
    {
        return (type == RS_NULL_KW_ID && ((optional && array_count == 0) || isFinallyOptional())) // for null comparisons
            || (generic && array_count == 0)
            || (type == type_id && array_count == 0);
    }
    inline bool canConvertTo(const rs_type_info& other) const
    {
        return !generic &&
               other.type_id == type_id         &&
               other.array_count == array_count &&
               typeDecoratorsEqualOrConvertable({isFinallyOptional(), reference}, {other.isFinallyOptional(), other.reference});
    }
    inline bool operator==(const rs_type_info& rhs) const
    { return equals(rhs); }
    constexpr static inline bool typeDecoratorsEqualOrConvertable(const std::pair<bool, bool> lhs, const std::pair<bool, bool> rhs)
    {
        // first index is optional, second is strict
        return ((rhs.first && !lhs.second) || rhs.first == lhs.first || rhs.second == lhs.second);
    }
    static inline void resolveGenericsIn(rs_type_info& info, const std::vector<rs_type_info>& generics, bool _explicit = true)
    {
        if (info.generic && info.generic_id >= 0 && info.generic_id < (int)generics.size())
        {
            const rs_type_info& t = generics[info.generic_id];
            info.assignType(t, _explicit);

        }
    }
    inline void assignType(const rs_type_info& t, bool _explicit = true)
    {
        // T[] int[]
        bool likemindedTypes = !_explicit 
            && generic 
            && t.array_count >= array_count;

        if (likemindedTypes)
        {
            // We are in the case: param = T[], arg = int[] -> T = int
            type_id = t.type_id;
            // Keep the generic identity
            generic = t.generic;
            generic_id = t.generic_id;
            return;
        }

        // Otherwise, assign the type directly
        array_count += t.array_count;
        if (t.arrayFlags.size() > 0)
            arrayFlags.insert(arrayFlags.begin(), t.arrayFlags.begin(), t.arrayFlags.end());
        
        type_id = t.type_id;

        // T? -> int[]?
        // T?[] -> int?[]?
        if (array_count > 0 && optional && t.isFinallyOptional())
        {
            optional = t.optional;
            arrayFlags.back().first = true;
        }

        // Keep the generic identity
        generic = t.generic;
        generic_id = t.generic_id;
    }

};

// oh lord
template <>
struct std::formatter<std::vector<rs_type_info>> : std::formatter<std::string> {
    auto format(const std::vector<rs_type_info>& vec, format_context& ctx) const {
        std::string out;
        for (size_t i = 0; i < vec.size(); ++i) {
            out += vec[i].tostr();
            if (i != vec.size() - 1)
                out += ", ";
        }
        return std::formatter<std::string>::format(out, ctx);
    }
};
// holy jesus
namespace std {
    template<>
    struct hash<rs_type_info> {
        std::size_t operator()(const rs_type_info& type) const {
            std::size_t h = 0;
            hash_combine(h, type.type_id);
            hash_combine(h, type.array_count);
            hash_combine(h, type.optional);
            hash_combine(h, type.reference);
            hash_combine(h, type.generic);
            hash_combine(h, type.generic_id);
            for (const auto& other : type.otherTypes) {
                hash_combine(h, std::hash<rs_type_info>{}(other));
            }
            return h;
        }

    private:
        template <typename T>
        static void hash_combine(std::size_t& seed, const T& value) {
            std::hash<T> hasher;
            seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
    };

    template<>
    struct hash<std::vector<rs_type_info>> {
        std::size_t operator()(const std::vector<rs_type_info>& vec) const {
            std::size_t h = 0;
            for (const auto& item : vec) {
                hash_combine(h, std::hash<rs_type_info>{}(item));
            }
            return h;
        }

    private:
        static void hash_combine(std::size_t& seed, std::size_t value) {
            seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
    };
}