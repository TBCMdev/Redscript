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
    OBJECT_ACCESS_OPERATOR,
    COMPARE_EQUAL,
    COMPARE_NOTEQUAL,
    MODULE_ACCESS,

    SYMBOL,
    
    TYPE_DEF,
    LINE_END,
    
    /* -- KEYWORDS -- */
#define TT_KEYWORDS_BEGIN static_cast<int>(token_type::KW_OR)
    KW_OR, KW_AND, KW_NOT,
    KW_TRUE, KW_FALSE,


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

#define TT_KEYWORDS_END static_cast<int>(token_type::KW_NULL)
};

namespace tutil
{
    inline constexpr std::string type_to_str(const token_type& tt)
    {
        switch (tt)
        {
            case token_type::WORD: return "word";
            case token_type::INT_LITERAL: return "int literal";
            case token_type::STRING_LITERAL: return "string literal";
            case token_type::FLOAT_LITERAL: return "float literal";
            case token_type::LIST_LITERAL: return "list literal";
            case token_type::OBJECT_LITERAL: return "object literal";
            case token_type::SELECTOR_LITERAL: return "selector literal";

            case token_type::COMMENT_INLINE: return "inline comment";
            case token_type::COMMENT_MULTILINE: return "multiline comment";

            case token_type::BRACKET_OPEN: return "bracket open";
            case token_type::BRACKET_CLOSED: return "bracket closed";
            case token_type::CBRACKET_OPEN: return "curly bracket open";
            case token_type::CBRACKET_CLOSED: return "curly bracket closed";
            case token_type::SQBRACKET_OPEN: return "square bracket open";
            case token_type::SQBRACKET_CLOSED: return "square bracket closed";
            case token_type::UNKNOWN: return "unknown";

            case token_type::OPERATOR: return "operator";
            case token_type::VAR_OPERATOR: return "variable operator";
            case token_type::COMPARE_EQUAL: return "==";
            case token_type::COMPARE_NOTEQUAL: return "!=";
            case token_type::MODULE_ACCESS: return "::";

            case token_type::SYMBOL: return "symbol";

            case token_type::KW_OR: return "or";
            case token_type::KW_AND: return "and";
            case token_type::KW_NOT: return "not";
            case token_type::KW_TRUE: return "true";
            case token_type::KW_FALSE: return "false";

            case token_type::TYPE_DEF: return "type definition";

            case token_type::KW_INT_TYPE: return "int";
            case token_type::KW_FLOAT_TYPE: return "float";
            case token_type::KW_BOOL_TYPE: return "bool";
            case token_type::KW_STRING_TYPE: return "string";
            case token_type::KW_LIST_TYPE: return "list";
            case token_type::KW_OBJECT_TYPE: return "object";
            case token_type::KW_ANY_TYPE: return "any";

            case token_type::KW_MODULE: return "module";
            case token_type::KW_RETURN: return "return";
            case token_type::KW_METHOD: return "method";
            case token_type::KW_USE: return "use";
            case token_type::KW_IF: return "if";
            case token_type::KW_ELSE: return "else";
            case token_type::KW_ELIF: return "elif";

            case token_type::KW_CONST: return "const";
            case token_type::KW_OPTIONAL: return "optional";
            case token_type::KW_REQUIRED: return "required";
            case token_type::KW_SEPERATE: return "separate";

            case token_type::KW_FOR: return "for";
            case token_type::KW_WHILE: return "while";
            case token_type::KW_IN: return "in";
            case token_type::KW_BREAK: return "break";
            case token_type::KW_CONTINUE: return "continue";

            case token_type::KW_ASM: return "asm";
            case token_type::KW_NULL: return "null";

            case token_type::LINE_END: return "semicolon";

            default: return "unrecognized";
        }
    }
    inline constexpr std::string type_to_str_full(const token_type& tt)
    {
        const int index = static_cast<int>(tt);
        const std::string result = type_to_str(tt);
        if (index >= TT_KEYWORDS_BEGIN && index <= TT_KEYWORDS_END)
            return '\'' + result + "' keyword.";
        return result;
    }
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