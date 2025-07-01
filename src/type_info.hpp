#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <functional>

#include "constants.hpp"

struct rs_type_info
{
    
    int32_t type_id     = -1; // first type id
    uint32_t array_count = 0;

    bool optional = false;
    bool strict   = false;

    bool    generic    = false;
    int32_t generic_id = -1;

    std::vector<rs_type_info> otherTypes = {}; // others if specified
    //              arrOptional, arrStrict

    std::vector<std::pair<bool, bool>> arrayFlags;

    inline std::string tostr() const
    {
        std::string typestr = std::to_string(type_id);
        if (strict)   typestr.push_back('!');

        for(size_t i = 0; i < otherTypes.size(); i++)
            typestr += '|' + otherTypes.at(i).tostr();

        if (array_count != 0)
            typestr += '[' + std::to_string(array_count) + ']';  
            
        if (optional) typestr.push_back('?');
        
        return typestr;
    }
    inline rs_type_info element_type() const
    {
        if (array_count < 1) return *this;

        return rs_type_info{type_id, array_count - 1, optional, strict, generic, generic_id, otherTypes, arrayFlags};
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
    inline bool equals(const rs_type_info& other) const
    {
        const bool aeq = array_count == other.array_count && compareArrayFlags(other);

        return ((generic && !other.generic && aeq)
            ||  (generic_id == other.generic_id && aeq))
            || (optional && other.type_id == RS_NULL_KW_ID)
            || (other.type_id == type_id && aeq && other.optional == optional && other.strict == strict)
            || canConvertTo(other);
    }
    inline bool equals(int32_t type) const
    {
        // int? -> int? : yes
        // int -> int? : yes
        // int -> int : yes
        // int? -> int : no
        // int -> int!

        return (generic && array_count == 0) || (type == type_id && array_count == 0);
    }
    inline bool canConvertTo(const rs_type_info& other) const
    {
        return !generic &&
               other.type_id == type_id         &&
               other.array_count == array_count &&
               typeDecoratorsEqualOrConvertable({optional, strict}, {other.optional, other.strict});
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
            info.type_id = t.type_id;
            // maybe needs something added here

            if (_explicit)
            {
                info.array_count += t.array_count;
                info.arrayFlags.insert(info.arrayFlags.begin(), t.arrayFlags.begin(), t.arrayFlags.end());
            }

        }
        for (auto& other : info.otherTypes)
            resolveGenericsIn(other, generics);
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
            hash_combine(h, type.strict);
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

    // ALSO define hash for vector<rs_type_info>
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