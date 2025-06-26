g++ -o rscript.exe src/*.cpp entry.cpp -I./src/libs -Isrc -g -std=c++20 -static -pipe
echo %ERRORLEVEL%