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
#include <unordered_map>
#include <type_traits>
#include <unordered_set>

#define UNUSED [[maybe_unused]]


inline constexpr std::string color_format(const std::string& _msg, const std::string& color, const std::string& reset_color)
{
    std::string copy = _msg;

    size_t caret = copy.find("{}");

    const size_t offset = color.size() + reset_color.size();

    while (caret != std::string::npos)
    {

        copy.replace(caret, 2, color + "{}" + reset_color);

        caret = copy.find("{}", caret + offset + 2);
    }
 
    return copy;
}

template<typename _Ty>
struct wrapper
{
    _Ty val;
};

template<typename _Ty>
struct shared_wrapper
{
    using _El = wrapper<std::shared_ptr<_Ty>>;
    std::shared_ptr<_El> element;

    constexpr shared_wrapper() noexcept : element() {} explicit
    constexpr shared_wrapper(std::nullptr_t) noexcept : element(nullptr) {}

    template<typename... _Args>
    shared_wrapper(_Args&&... args) : element(std::make_shared<_El>(args...)) {}

    shared_wrapper& operator=(const std::shared_ptr<_Ty>& nonWrapperLike)
    {
        element->val = nonWrapperLike;

        return *this;
    }
    _Ty& operator*()
    {
        return *element->val;
    }
    _Ty* operator->()
    {
        return element->val.get();
    }
};
#pragma region weak_ptr
/*
THE BELOW IMPL HAS BEEN DEFINED IN C++26
*/
template<typename T>
struct weak_ptr_hash {
    std::size_t operator()(const std::weak_ptr<T>& wp) const noexcept {
        if (auto sp = wp.lock())
            return std::hash<T*>{}(sp.get());
        return std::hash<T*>{}(nullptr);
        
    }
};

// Equality comparator for weak_ptr
template<typename T>
struct weak_ptr_equal {
    bool operator()(const std::weak_ptr<T>& lhs, const std::weak_ptr<T>& rhs) const noexcept {
        return lhs.lock().get() == rhs.lock().get();
    }
};
/*
@deprecated 
*/
#pragma endregion weak_ptr
template<typename _Ty>
struct linked_ptr : std::enable_shared_from_this<linked_ptr<_Ty>>
{
    using shared    = std::shared_ptr<_Ty>;

    using conn_item = std::weak_ptr<linked_ptr<_Ty>>;
    using conn_list = std::unordered_set< conn_item, weak_ptr_hash<linked_ptr<_Ty>>, weak_ptr_equal<linked_ptr<_Ty>> >;

    std::shared_ptr< _Ty       > _M_ptr         = nullptr;
    std::shared_ptr< conn_list > _M_connections = nullptr;

    linked_ptr(const shared& other) : _M_ptr(other)
    {}
    
    linked_ptr(const _Ty& val) : _M_ptr(std::make_shared<_Ty>(val)) {}

    linked_ptr(const linked_ptr<_Ty>& other) : shared(other.shared_from_this())
    { *this = other; }
    linked_ptr() = default;
    
    void _create_links()
    {
        _M_connections = std::make_shared<conn_list>();
        _M_connections->insert(this->shared_from_this());
    }

    void _update_links()
    {
        if (!_M_connections) _create_links();

        typename conn_list::iterator iter = _M_connections->begin();
        while (iter != _M_connections->end())
        {
            if (auto ptr = iter->lock() && ptr != this)
            {
                *ptr = *this;
                ++iter;
            }
            else
                iter = _M_connections->erase(iter);
        }

    }
    void merge_connections(const linked_ptr<_Ty>& other)
    {
        if (!_M_connections) _create_links();
        
        if (!other._M_connections || other._M_connections->empty())
        {
            _M_connections->insert(other.shared_from_this());
            return;
        }

        if (_M_connections == other._M_connections) return;

        _M_connections->merge(*other._M_connections);
    }
    linked_ptr<_Ty>& operator=(const linked_ptr<_Ty>& other)
    {
        _M_ptr = other._M_ptr;
        merge_connections(other);

        _update_links();

        return *this;
    }
    _Ty& operator*()
    {
        return *_M_ptr;
    }
    _Ty& operator->()
    {
        return *_M_ptr;
    }
};

template<typename _Ty>
using linked_ptr_instance = std::shared_ptr<linked_ptr<_Ty>>;

template<typename _Ty, typename... _Args>
inline constexpr linked_ptr_instance<_Ty> make_linked(_Args&&... args)
{
    return std::make_shared<linked_ptr<_Ty>>(args...);
}

template<typename _FlagT, typename _FlagV>
struct flag_list
{
    std::unordered_map<_FlagT, _FlagV> flags;

    flag_list(std::unordered_map<_FlagT, _FlagV> Flags) : flags(Flags)
    {}

    inline _FlagV& get(const _FlagT& x) const
    {
        return flags.find(x)->second;
    }
    inline bool exists(const _FlagT& x) const
    {
        return flags.find(x) != flags.end();
    }
    inline void set(const _FlagT& x, const _FlagV& v)
    {
        flags.insert_or_assign({x, v});
    }

};

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

/*
Returns a result pair (true or false along with the ordered native values) if lhs and rhs both equal lhsval or rhsval, granted
they don't equal the same value.
*/
template<typename _ComparisonT, typename _ValueT>
inline constexpr result_pair<_ValueT, _ValueT> commutativeEquals(const _ComparisonT& lhs, const _ComparisonT& lhsval,
                                                                 const _ComparisonT& rhs, const _ComparisonT& rhsval,
                                                                 _ValueT& lhsnative,
                                                                 _ValueT& rhsnative)
{
    if (lhs == lhsval && rhs == rhsval)
        return result_pair(lhsnative, rhsnative);
    if (rhs == lhsval && lhs == rhsval)
        return result_pair(rhsnative, lhsnative);

    return result_pair<_ValueT, _ValueT>();
}

struct EnumPairHash {
    template <typename T1, typename T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
        using U1 = std::underlying_type_t<T1>;
        using U2 = std::underlying_type_t<T2>;

        std::size_t h1 = std::hash<U1>{}(static_cast<U1>(p.first));
        std::size_t h2 = std::hash<U2>{}(static_cast<U2>(p.second));

        return h1 ^ (h2 << 1);
    }
};

template<typename _KeyValT, typename _ValT, typename _Hasher>
using pairedkey_map_t = std::unordered_map<std::pair<_KeyValT, _KeyValT>, _ValT, _Hasher>;
template<typename _KeyValT, typename _ValT>
using pairedkey_map   = std::unordered_map<std::pair<_KeyValT, _KeyValT>, _ValT, EnumPairHash>;

template<typename _ValueT>
using combination_functor = std::function<void(const _ValueT&, const _ValueT&)>;

// will only call 1 key,val pair.
template<typename _ComparisonT, typename _ValueT>
inline constexpr bool combinationCommutativeEquals(const _ComparisonT& lhs,
                                                   const _ComparisonT& rhs,
                                                   _ValueT& lhsnative,
                                                   _ValueT& rhsnative,
                                                   const pairedkey_map
                                                        <_ComparisonT, combination_functor<_ValueT>>&
                                                                       vals)
{
    for(const auto& [key, val] : vals)
    {
        auto result = commutativeEquals(lhs, key.first, rhs, key.second, lhsnative, rhsnative);
        if (result)
        {
            val(lhsnative, rhsnative);
            return true;
        }
    }
    return false;
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

    T& at(size_t index) const
    {
        return this->c.at(index);
    }
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
