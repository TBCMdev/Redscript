#include "error.hpp"
#include "constants.hpp"
#include "token.hpp"
std::string syntaxHighlight(const std::string& s, const std::string color)
{
    static const std::unordered_map<std::string, std::tuple<token_type, uint32_t>> KEYWORDS = RS_LANG_KEYWORDS;
    static const std::string HIGHLIGHT_COLOR_STR = KEYWORD_HIGHLIGHT_COLOR;
    static const std::string HIGHLIGHT_TYPE_COLOR_STR = TYPE_HIGHLIGHT_COLOR;

    std::string result = s;

    for (const auto& keyword : KEYWORDS)
    {
        const std::string& word = keyword.first;
        size_t at = 0;

        while ((at = result.find(word, at)) != std::string::npos)
        {
            const size_t S = word.size();
            if ((at > 0 && std::isalpha(result.at(at - 1))) || (at + S < result.size() && std::isalpha(result.at(at + S))))
            {
                at += S;
                continue;
            }
            result.replace(at, S,
                (std::get<0>(keyword.second) == token_type::TYPE_DEF ? HIGHLIGHT_TYPE_COLOR_STR : HIGHLIGHT_COLOR_STR)
                + word + color);
            at += HIGHLIGHT_COLOR_STR.size() + S + color.size(); // move past inserted text
        }
    }

    return result;
}

void printerr(rs_error& error, std::vector<std::string> notes)
{
    std::stringstream fileStr;
    fileStr << error.fName << ':' << error.trace.line << ':' << error.trace.caret;
    ERROR("[RS:%d] %s", error.trace.ec, error.message.c_str());

    for (std::string& note : notes)
        std::cout << ERROR_NOTE_COLOR << "  [NOTE] " << note << ERROR_RESET "\n";
    
    if (error.callTrace && error.callTrace->size() > 0)
    {
        auto& callTrace = *error.callTrace;
        ERROR("Stack trace: ");
        size_t at = callTrace.size();
        std::cout << GRAY_COLOR;
        for(auto& funcStr : callTrace)
        {
            std::cout << "\t " << at << " : " << syntaxHighlight(funcStr, GRAY_COLOR) << '\n';
            at --;
        }

        std::cout << ERROR_RESET;
    }

    std::cout << "\n\t -- " << fileStr.str() << " -- \n";
    for(size_t i = 0; i < std::min(error.trace.line - RS_ERROR_LINE_PADDING + 1, (size_t)RS_ERROR_LINE_PADDING); i++)
        std::cout << "      |\n";

    std::stringstream errorHighlight;
    bool hasEnd = error.trace.start != -1;
    if (hasEnd)
    {
        for(long long i = 1; i < error.trace.start; i++) errorHighlight << ' ';
        for(size_t i = error.trace.start; i < error.trace.caret + 1; i++) errorHighlight << '^';
    }
    else
    {
        for(size_t i = 1; i < error.trace.caret; i++) errorHighlight << ' ';
        errorHighlight << '^';
    }
    errorHighlight << " error here";

    int lineLength = std::to_string(error.trace.line).length();

    std::string paddl, paddr;
    for(int i = 0; i < 3 - lineLength; i++) paddl.push_back(' ');
    for(size_t i = 0; i < 6 - lineLength - paddl.length(); i++) paddr.push_back(' ');

    std::cout << paddl << error.trace.line << paddr << "| " << syntaxHighlight(error.line) << "\n      | " << ERROR_COLOR << errorHighlight.str() << ERROR_RESET << '\n';
    for(int i = 0; i < RS_ERROR_LINE_PADDING - 1; i++)
        std::cout << "      |\n";
}