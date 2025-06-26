set CXX=g++
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build

.\build\rscript.exe %* "Redscript - testing"