#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <format>

#include "error.hpp"

enum class token_type
{
    WORD,
    INT_LITERAL,
    STRING_LITERAL,
    FLOAT_LITERAL,
    LIST_LITERAL,
    OBJECT_LITERAL,
    SELECTOR_LITERAL, // @<name>

    COMMENT_INLINE,
    COMMENT_MULTILINE,
    
    BRACKET_OPEN,
    BRACKET_CLOSED,
    CBRACKET_OPEN,
    CBRACKET_CLOSED,
    SQBRACKET_OPEN,
    SQBRACKET_CLOSED,
    UNKNOWN,
    
    /* -- SYMBOL GROUPS -- */
    OPERATOR,
    VAR_OPERATOR,
    COMPARE_EQUAL,
    COMPARE_NOTEQUAL,
    MODULE_ACCESS,

    SYMBOL,

    /* -- KEYWORDS -- */
    KW_OR, KW_AND, KW_NOT,
    KW_TRUE, KW_FALSE,

    TYPE_DEF,

    KW_INT_TYPE,
    KW_FLOAT_TYPE,
    KW_BOOL_TYPE,
    KW_STRING_TYPE,
    KW_LIST_TYPE,
    KW_OBJECT_TYPE,
    KW_ANY_TYPE,
    KW_MODULE,
    KW_RETURN,
    KW_METHOD,
    KW_USE,
    KW_IF,
    KW_ELSE,
    KW_ELIF,

    KW_CONST,
    KW_OPTIONAL,
    KW_REQUIRED,
    KW_SEPERATE,

    KW_FOR,
    KW_WHILE,
    KW_IN,
    KW_BREAK,
    KW_CONTINUE,

    KW_ASM,
    KW_NULL,

    LINE_END

};

struct token
{
    std::string repr;
    token_type  type;
    int32_t     info = -1;

    raw_trace_info trace;

    std::string str()
    {
        if (type == token_type::STRING_LITERAL)
            return std::format("{{\"{}\", {}, {}, {}}}", repr, static_cast<int>(type), info, trace.caret);
        return std::format("{{{}, {}, {}, {}}}", repr, static_cast<int>(type), info, trace.caret);
    }
    operator std::string()
    {
        return repr;
    }
    token(std::string _repr, token_type _type, uint32_t _info, raw_trace_info _trace, long start)
        : repr(_repr), type(_type), info(_info), trace(_trace)
    {
        trace.start = start - trace.nlindex;
        if (trace.start > -1 && static_cast<size_t>(trace.start) == trace.caret) trace.start = -1;
    }
    token(std::string _repr, token_type _type, uint32_t _info, raw_trace_info _trace)
        : repr(_repr), type(_type), info(_info), trace(_trace) {}
};

typedef std::vector<token> token_list;