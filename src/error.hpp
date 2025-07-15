#pragma once
#include <cstdint>
#include <format>
#include <iostream>
#include <sstream>
#include <memory>
#include <unordered_map>
#include <vector>
#include "logger.hpp"

#define RS_ERROR_LINE_PADDING 1
#include "errors.hpp"


#include "util.hpp"

struct raw_trace_info
{
    size_t at = 0, line = 0, caret = 0, nlindex = 0;
    long long start = -1;
};

struct stack_trace
{
    uint32_t ec = 0;
    std::shared_ptr<size_t> at = nullptr;
    size_t line = 1, caret = 0, nlindex = 0;
    long long start=-1;

    operator raw_trace_info ()
    {
        return raw_trace_info{*at, line, caret, nlindex, start};
    }
};
struct rs_error
{
    stack_trace trace;
    std::string message, line, fName;
    std::shared_ptr<std::string> content = nullptr;
    std::shared_ptr<std::vector<std::string>> callTrace = nullptr;
    
    template<typename... _Args>
    rs_error(const std::string& _message,
             const std::string& _content,
             stack_trace        _trace,
             std::string        _fName,
             _Args&&...         _variables) :
                    trace(_trace),
                    message(std::vformat(
                        std::string_view(_message),
                        std::make_format_args(static_cast<const std::remove_reference_t<_Args>&>(_variables)...)
                    )),
                    fName(_fName),
                    content(std::make_shared<std::string>(_content))
    {
        _setLine();
    }
    template<typename... _Args>
    rs_error(const std::string& _message,
            const std::string&  _content,
            raw_trace_info&    _raw,
            std::string        _fName,
            _Args&&...         _variables) :
               trace{0, std::make_shared<size_t>(_raw.at), _raw.line, _raw.caret, _raw.nlindex, _raw.start},
               message(std::vformat(
                        std::string_view(_message),
                        std::make_format_args(static_cast<const std::remove_reference_t<_Args>&>(_variables)...)
                    )),
               fName(_fName),
               content(std::make_shared<std::string>(_content))
    {
        _setLine();
    }
    template<typename... _Args>
    rs_error(const std::string& _message,
             const std::string& _content,
             stack_trace        _trace,
             std::shared_ptr<std::vector<std::string>> _callTrace,
             std::string        _fName,
             _Args&&...         _variables) :
                    trace(_trace),
                    message(std::vformat(
                        std::string_view(_message),
                        std::make_format_args(static_cast<const std::remove_reference_t<_Args>&>(_variables)...)
                    )),
                    fName(_fName),
                    content(std::make_shared<std::string>(_content)),
                    callTrace(_callTrace)
    {
        _setLine();
    }
    template<typename... _Args>
    rs_error(const std::string& _message,
             const std::string& _content,
             raw_trace_info&    _raw,
             std::shared_ptr<std::vector<std::string>> _callTrace,
             std::string        _fName,
             _Args&&...         _variables) :
                    trace{0, std::make_shared<size_t>(_raw.at), _raw.line, _raw.caret, _raw.nlindex, _raw.start},
                    message(std::vformat(
                        std::string_view(_message),
                        std::make_format_args(static_cast<const std::remove_reference_t<_Args>&>(_variables)...)
                    )),
                    fName(_fName),
                    content(std::make_shared<std::string>(_content)),
                    callTrace(_callTrace)
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

std::string syntaxHighlight(const std::string& s, const std::string col = ERROR_RESET);
void printerr(rs_error&, std::vector<std::string> = {});