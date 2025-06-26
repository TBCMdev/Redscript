#pragma once
#include <cstdint>
#include <format>
#include <iostream>
#include <sstream>
#include <memory>
#include "logger.hpp"

#define RS_ERROR_LINE_PADDING 2
#include "errors.hpp"

struct raw_trace_info
{
    long at = 0, line = 0, caret = 0, nlindex = 0, start=-1;
};

struct stack_trace
{
    uint32_t ec = 0;
    std::shared_ptr<long> at = nullptr;
    long line = 1, caret = 0, nlindex = 0, start=-1;

    operator raw_trace_info ()
    {
        return raw_trace_info{*at,line,caret,nlindex,start};
    }
};
struct rs_error
{
    stack_trace trace;
    std::string message, line, fName;
    std::shared_ptr<std::string> content = nullptr;
    template<typename... _Args>
    rs_error(const std::string& _message,
             std::string&       _content,
             stack_trace        _trace,
             std::string        _fName,
             _Args&&...         _variables) :
                    content(std::make_shared<std::string>(_content)),
                    trace(_trace),
                    fName(_fName),
                    message(std::vformat(_message, std::make_format_args(std::forward<_Args>(_variables)...)))
    {
        _setLine();
    }
    template<typename... _Args>
    rs_error(const std::string& _message,
            std::string&       _content,
            raw_trace_info&    _raw,
            std::string        _fName,
            _Args&&...         _variables) :
               content(std::make_shared<std::string>(_content)),
               trace{0, std::make_shared<long>(_raw.at), _raw.line, _raw.caret, _raw.nlindex, _raw.start},
               fName(_fName),
               message(std::vformat(_message, std::make_format_args(std::forward<_Args>(_variables)...)))
    {
        _setLine();
    }
    rs_error(){}


private:
    void _setLine()
    {
        std::string& _content = *content;

        size_t at = *trace.at;
        size_t L  = _content.length();

        while (++at < L && _content.at(at) != '\n');

        // dont append first \n and last \n to line
        line = _content.substr(trace.nlindex > 0 ? trace.nlindex + 1 : 0, at - trace.nlindex - 1);
    }
};

void printerr(rs_error&);
