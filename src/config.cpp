#include "config.hpp"

#define CONFIG_ERROR(message, ...)                                    \
    {                                                                    \
        *err = rs_error(message, content, trace, "rs.config", ##__VA_ARGS__);  \
        err->trace.ec = RS_CONFIG_ERROR;                                                  \
        return config;                                                   \
    }
rs_config readConfig(const std::string& path, rs_error* err)
{
    rs_config config;

    std::string content = readFile(path);
    stack_trace trace;
    if(content.empty())
        CONFIG_ERROR("RS config does not exist.");

    const size_t S = content.size();
    std::shared_ptr<size_t> _At = std::make_shared<size_t>(0);
    size_t& iter = *_At;
    while((iter = content.find('=', iter + 1)) != std::string::npos)
    {
        trace.line++;
        size_t start = iter, end = iter;

        while (start > 0 && content.at(start - 1) != '\n') start--;
        while (end + 1 < S && content.at(end + 1) != '\n') end++;
        trace.nlindex = start;
        trace.start = start;
        trace.caret = end;

        std::string flag = content.substr(start, iter - start);
        std::string value;
        if (end == iter) value = "";
        else             value = content.substr(iter + 1, end - iter + 1);
        if (value.back() == '\n')
            value.pop_back();
        if (std::isdigit(value.at(0)))
        {
            try{
                config.dict.insert_or_assign(flag, std::stoi(value));
            }catch(std::exception& e)
                CONFIG_ERROR("Expected int.");
        }
        else
            config.dict.insert_or_assign(flag, value);
    }
    return config;
}
#undef CONFIG_ERROR