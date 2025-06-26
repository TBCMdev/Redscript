#include <iostream>
#include <cstdint>
#include "lexer.hpp"
#include "file.hpp"
#include "logger.hpp"
#include "rbc.hpp"
#include "config.hpp"
#include "getopt.h"
int main(int argc, char* const* argv)
{
    if (argc < 3) 
    {
        ERROR("Invalid command line arguments supplied (argc: %d).", argc);
        return EXIT_FAILURE;
    }
#pragma region ARGS
    char* fileName   = nullptr;
    const char* outFolder  = nullptr;
    bool debug       = false;
    int opt;
    while ((opt = getopt(argc, argv, "f:o:d")) != -1)
    {
        switch (opt)
        {
            case 'd':
                debug = true;
                break;
            case 'f':
                fileName = optarg;
                break;
            case 'o':
                outFolder = optarg;
                break;
            case '?':
                ERROR("Unknown option: %s", optarg);
                return 1;
        }
    }

    if (!fileName)
    {
        if (optind < argc)
            fileName = argv[optind++];
        else 
        {
            ERROR("Usage: %s <fname> <outfolder> <args...>", argv[0]);
            return EXIT_FAILURE;
        }
    }
    if (!outFolder)
    {
        if (optind < argc)
            outFolder = argv[optind++];
        else 
        {
            ERROR("Usage: %s <fname> <outfolder> <args...>", argv[0]);
            return EXIT_FAILURE;
        }
    }

#pragma endregion ARGS
    rs_error error;

    RS_CONFIG = readConfig(RS_CONFIG_LOCATION, &error);
    if(error.trace.ec)
    {
        printerr(error);
        return EXIT_FAILURE;
    }

    std::string fContent = readFile(fileName);

    if(fContent.length() == 0)
    {
        ERROR("Provided source file does not exist.");
        return EXIT_FAILURE;
    }

    token_list list = tlex(fileName, fContent, &error);

    if (error.trace.ec)
    {
        printerr(error);
        return EXIT_FAILURE;
    }
    INFO("Preprocessing...");

    preprocess(list, fileName, fContent, &error);

    if(error.trace.ec)
    {
        printerr(error);
        return EXIT_FAILURE;
    }

    if(debug || 1)
    {
        INFO("Token Count: %zu", list.size());
        for(token t : list)
        {
            std::cout << t.str() << std::endl;
        }
    }
    
    INFO("Compiling...");

    rbc_program bytecode = torbc(list, fileName, fContent, &error);

    if (error.trace.ec)
    {
        printerr(error);
        return EXIT_FAILURE;
    }
    int i = 1;
    if (debug || 1)
    {
        std::ofstream out("./out.rbc");
        INFO("Writing global byte code to out.rbc...");
        for(auto& function   : bytecode.functions)
        {
            out << function.second->toHumanStr() << '\n';
        }
        for(rbc_command& instruction : bytecode.globalFunction.instructions)
        {
            INFO("[scope: GLOBAL] [%d] %s", i, instruction.tostr().c_str());
            out << instruction.toHumanStr() << '\n';
            i++;
        }

        out.close();

    }
    std::string conversionError = "";
    std::string outFolderLower = outFolder;

    toLower(outFolderLower);

    // mc_program endProgram = tomc(bytecode, removeSpecialCharacters(outFolderLower), conversionError);
    mc_program endProgram = tomc(bytecode, "redscript", conversionError);

    if (!conversionError.empty())
    {
        ERROR("%s", conversionError.c_str());
        return EXIT_FAILURE;
    }
    
    std::string packageName = removeSpecialCharacters(std::filesystem::path(outFolder).filename().string());
    writemc(endProgram, packageName, outFolderLower, conversionError);

    if (!conversionError.empty())
    {
        ERROR("Error while writing: %s", conversionError.c_str());
        return EXIT_FAILURE;
    }
    SUCCESS("Compiled successfully to %s.", outFolder);
    return EXIT_SUCCESS;
}