#pragma once
#include <string>
#include <deque>
#include <memory>

#include "../token.hpp"

struct project_fragment
{
    std::string fileName;
    std::string fileContent;

    token_list tokens;
};

typedef std::deque<std::shared_ptr<project_fragment>> fragment_ptr_deque;
