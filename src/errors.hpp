#pragma once

#define RS_UNKNOWN                    -1
#define RS_SYNTAX_ERROR                1
#define RS_EOF_ERROR                   2
#define RS_UNSUPPORTED_OPERATION_ERROR 3
#define RS_ALREADY_INCLUDED_ERROR      4
#define RS_CONFIG_ERROR                5
#define RS_BYTE_CODE_ERROR             6
#define RS_CPP_IMPL_ERROR              7


#define ABORT_PARSE throw program.context

#define EXPR_ERROR_R(_ec, _message, _trace, ret, ...)                             \
    {                                                                             \
        program.context = new rs_error(_message, program.currentFragment->fileContent, _trace, std::make_shared<std::vector<std::string>>(program.callStackStr()), program.currentFragment->fileName, ##__VA_ARGS__);  \
        program.context->trace.ec = _ec;                                          \
        ABORT_PARSE;                                                    \
    }

#define COMP_ERROR(_ec, message, ...)                                    \
    {                                                                    \
        err = rs_error(message, *content, currentToken->trace, std::make_shared<std::vector<std::string>>(program.callStackStr()), currentFile->fileName, ##__VA_ARGS__);  \
        err.trace.ec = _ec;                                             \
        ABORT_PARSE;                                                     \
    }
#define COMP_ERROR_R(_ec, message, ret, ...)                             \
    {                                                                    \
        err = rs_error(message, *content, currentToken->trace, std::make_shared<std::vector<std::string>>(program.callStackStr()), currentFile->fileName, ##__VA_ARGS__);  \
        err.trace.ec = _ec;                                             \
        ABORT_PARSE;                                                     \
    }
#define COMP_ERROR_T(_ec, message, _trace, ...)                             \
    {                                                                    \
        err = rs_error(message, *content, _trace, std::make_shared<std::vector<std::string>>(program.callStackStr()), currentFile->fileName, ##__VA_ARGS__);  \
        err.trace.ec = _ec;                                             \
        ABORT_PARSE;                                                     \
    }
