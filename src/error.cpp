#include "error.hpp"

void printerr(rs_error& error)
{
    std::stringstream fileStr;
    fileStr << error.fName << ':' << error.trace.line << ':' << error.trace.caret;
    ERROR("[RS:%d] %s", error.trace.ec, error.message.c_str());
    std::cout << "\n\n\t -- " << fileStr.str() << " -- \n\n";
    for(size_t i = 0; i < std::min(error.trace.line - RS_ERROR_LINE_PADDING + 1, (size_t)RS_ERROR_LINE_PADDING); i++)
        std::cout << "      |\n";

    std::stringstream errorHighlight;
    bool hasEnd = error.trace.start != -1;
    if (hasEnd)
    {
        for(long long i = 0; i < error.trace.start; i++) errorHighlight << ' ';
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

    std::cout << paddl << error.trace.line << paddr << "| " << error.line << "\n      | " << ERROR_COLOR << errorHighlight.str() << ERROR_RESET << '\n';
    for(int i = 0; i < RS_ERROR_LINE_PADDING - 1; i++)
        std::cout << "      |\n";
}