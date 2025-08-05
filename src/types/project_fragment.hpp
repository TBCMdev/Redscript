#pragma once
#include <string>
#include <deque>
#include <memory>
#include <filesystem>
#include "../token.hpp"

struct project_fragment
{
    std::string fileName;
    std::string fileContent;
    token_list tokens;
    std::filesystem::path folderPath;
};

typedef std::deque<std::shared_ptr<project_fragment>> fragment_ptr_deque;
