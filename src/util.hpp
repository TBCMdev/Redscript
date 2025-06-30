#pragma once
#include <cctype> // for std::isalnum
#include <algorithm>
#include <stack>
#include <deque>

#include <iostream>
#include <string>
#include <functional>
#include <sstream>
#include <iomanip>

#include <type_traits>

inline std::string removeSpecialCharacters(const std::string &input)
{
    std::string output;
    for (char c : input)
    {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.')
        {
            output += c;
        }
        else if (c == '-')
        {
            output += '_';
        }
    }
    return output;
}
inline void toLower(std::string &str)
{
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });
}
template <typename T>
inline T &unmove(T &&x)
{
    return x;
}

template<typename T>
struct unwrapped_shared
{ using type = T;};


template<typename T>
struct unwrapped_shared<std::shared_ptr<T>>
{ using type = T;};

template<typename _T>
struct range
{
    _T low, high;

public:
    range(_T&& _low, _T&& _high) : low(_low), high(_high) {}
};

template <typename _T1, typename _T2>
struct  result_pair
{
    bool result;
    unwrapped_shared<_T1>::type* i1;
    unwrapped_shared<_T2>::type* i2;

    result_pair(_T1& _i1, _T2& _i2)
        : result(true),
          i1(unwrap_ptr(_i1)),
          i2(unwrap_ptr(_i2))
    {}

    result_pair() : result(false), i1(nullptr), i2(nullptr) {}

    operator bool() const { return result; }

private:

    template<typename _T>
    static _T* unwrap_ptr(std::shared_ptr<_T> x)
    { return x.get(); }

    template<typename _T>
    static _T* unwrap_ptr(_T& x)
    { return &x; }
    
};
template <typename _VariantT, typename _VariantT2, typename _Variant>
inline constexpr result_pair<_VariantT, _VariantT2> commutativeVariantEquals(size_t aval, _Variant &a, size_t bval, _Variant &b)
{
    if (a.index() == aval && b.index() == bval)
        return result_pair(std::get<_VariantT>(a), std::get<_VariantT2>(b));
    else if (a.index() == bval && b.index() == aval)
        return result_pair(std::get<_VariantT>(b), std::get<_VariantT2>(a));

    return result_pair<_VariantT, _VariantT2>();
}

template <typename T, typename Container = std::deque<T>>
class iterable_stack : public std::stack<T, Container>
{
public:
    typedef typename Container::iterator iterator;
    typedef typename Container::const_iterator const_iterator;

    iterator begin() { return this->c.begin(); }
    iterator end() { return this->c.end(); }
    const_iterator begin() const { return this->c.begin(); }
    const_iterator end() const { return this->c.end(); }
};

namespace util
{
    template <typename T>
    constexpr T copy(const T &t)
    {
        return T(t);
    }
    inline std::string hashToHex(const std::string &input)
    {
        std::hash<std::string> hasher;
        size_t hashValue = hasher(input);

        std::stringstream ss;
        ss << std::hex << std::setw(sizeof(size_t) * 2) << std::setfill('0') << hashValue;
        return ss.str();
    }
}
