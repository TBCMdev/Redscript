#include "lexer.hpp"
// ex:
// LEX_ERROR(RS_SYNTAX_ERROR, "Something went wrong because of the number {}", 44);
#define LEX_ERROR(_ec, message, ...)                                    \
    {                                                                   \
        trace.ec = _ec;                                                 \
        *err = rs_error(message, content, trace, fName, ##__VA_ARGS__); \
        return tokens;                                                  \
    }
#define LEX_ERRORF(_ec, message, start, ...)                                   \
    {                                                                          \
        trace.ec = _ec;                                                        \
        *err = rs_error(message, content, trace, fName, start, ##__VA_ARGS__); \
        return tokens;                                                         \
    }
token_list tlex(const std::string &fName, std::string &content, rs_error *err = nullptr)
{
    lex_info LEX_INFO;
    auto _At_ptr = std::make_shared<size_t>(0);
    size_t& _At = *_At_ptr;
    size_t S = content.length();

    stack_trace trace(0, _At_ptr);
    token_list tokens;

    trace.caret = -1;

    auto back = [&]() -> char
    {
        if (_At == 0) return 0;

        char c = content.at(--_At);

        if (c == '\n')
        {
            trace.line--;
            trace.nlindex = _At;
        }
        else
            trace.caret--;
        return c;
    };
    std::function<char()> adv = [&]() -> char
    {
        if (_At + 1 >= S)
            return 0;
        char current = content.at(_At);

        if (current == '\n')
        {
            trace.caret = 0;
            trace.line++;
            trace.nlindex = _At;
        }
        else
            trace.caret++;
        char c = content.at(++_At);

        return c;
    };
    char ch = content.at(_At);
    do
    {
        if (ch == '\n')
            continue;
        if (ch == '"' || ch == '\'')
        {
            long start = _At + 1;
            bool escaped = false;
            while ((ch = adv()))
            {
                if (ch == '\\')
                    escaped = true;
                else if (!escaped && (ch == '"' || ch == '\''))
                    break;
                else if (escaped)
                    escaped = false;
            }
            if (_At == S)
                LEX_ERRORF(RS_SYNTAX_ERROR, "Unterminated string-literal.", start);

            tokens.push_back(token{content.substr(start, _At - start), token_type::STRING_LITERAL, RS_STRING_KW_ID, trace, start});
        }
        else if (std::isdigit(ch))
        {
            long start = _At;
            bool decimal = false;

            while ((ch = adv()))
            {
                if (ch == '.')
                {
                    if (decimal)
                        LEX_ERROR(RS_SYNTAX_ERROR, "Invalid floating point notation.");
                    decimal = true;
                }
                else if (!std::isdigit(ch))
                    break;
            }
            token_type t = decimal ? token_type::FLOAT_LITERAL : token_type::INT_LITERAL;

            tokens.push_back(token{content.substr(start, _At - start),
                                   t, (uint32_t)(decimal ? RS_FLOAT_KW_ID : RS_INT_KW_ID),
                                   trace,
                                   start});
            back(); // go back 1 char
        }
        else if (ch == '_' || ch == '@' || std::isalpha(ch))
        {
            bool isSelectorLiteral = ch == '@';
            long start = isSelectorLiteral ? _At + 1 : _At;
            while ((ch = adv()) && (std::isalpha(ch) || ch == '_'));

            token t{content.substr(start, _At - start),
                isSelectorLiteral ? token_type::SELECTOR_LITERAL : token_type::WORD,
                (uint32_t)(isSelectorLiteral ? RS_SELECTOR_KW_ID : 0) ,trace, start};
            
            back(); // go back 1 char

            auto keyword = LEX_INFO.keywords.find(t.repr);

            if (keyword != LEX_INFO.keywords.end())
            {
                if (isSelectorLiteral) LEX_ERRORF(RS_SYNTAX_ERROR, "Expected selector literal, not keyword '{}'", start, t.repr);

                auto& tupl = keyword->second;
                t.type = std::get<0>(tupl);
                t.info = std::get<1>(tupl);
            }
            tokens.push_back(t);
        }
        else if (ch == '/' && _At + 1 < S && content.at(_At + 1) == '/')
        {
            adv();
            while((ch = adv()) && ch != '\n')
                if (ch == '\\' && _At + 1 < S && content.at(_At + 1) == '\n')
                    LEX_ERROR(RS_SYNTAX_ERROR, "A backslash cannot terminate a single lined comment.");
        }
        else if (ch == '/' && _At + 1 < S && content.at(_At + 1) == '*')
        {
            adv();
            bool found = false;
            while((ch = adv()))
            {
                if (ch == '\\')
                    adv();
                else if (ch == '*' && _At + 1 < S && content.at(_At + 1) == '/')
                {
                    found = true;    
                    break;
                }
            }
            if (!found)
                LEX_ERROR(RS_SYNTAX_ERROR, "Unterminated multi-line comment.");
            adv();
        }
        else
        {
            token_type customType = token_type::SYMBOL;
            std::string repr(1, ch);
            switch (ch)
            {
            case '\t':
            case ' ':
                continue;
            case ';':
                customType = token_type::LINE_END;
                break;
            case ':':
            {
                if (_At + 1 < S && content.at(_At + 1) == ':')
                {
                    adv();
                    customType = token_type::MODULE_ACCESS;
                    repr.push_back(':');
                }
                break;
            }
            case '+':
            case '-':
            case '*':
            case '/':
            case '%':
            {
                customType = token_type::OPERATOR;
                if (_At + 1 < S)
                {
                    char x = adv();
                    bool found = true;
                    switch(x)
                    {
                        case '=':
                            customType = token_type::VAR_OPERATOR;
                            break;
                        default:
                            back();
                            found = false;
                            break;
                    }
                    if (found)
                        repr.push_back(x);
                }
                break;
            }
            case '=':
            case '!':
            {
                bool n = ch == '!';
                if (_At + 1 < S)
                {
                    char x = adv();
                    bool found = true;
                    switch(x)
                    {
                        case '=':
                            customType = n ? token_type::COMPARE_NOTEQUAL : token_type::COMPARE_EQUAL;
                            break;
                        default:
                            back();
                            found = false;
                            break;
                    }
                    if (found)
                        repr.push_back(x);
                }
                break;
            }
            case '(':
                customType = token_type::BRACKET_OPEN;
                break;
            case ')':
                customType = token_type::BRACKET_CLOSED;
                break;
            case '[':
                customType = token_type::SQBRACKET_OPEN;
                break;
            case ']':
                customType = token_type::SQBRACKET_CLOSED;
                break;
            case '{':
                customType = token_type::CBRACKET_OPEN;
                break;
            case '}':
                customType = token_type::CBRACKET_CLOSED;
                break;
            }
            tokens.push_back(token{repr, customType, (uint32_t)ch, trace});
        }
    } while ((ch = adv()));

    return tokens;
}
#undef LEX_ERRORF
#undef LEX_ERROR
