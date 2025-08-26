set CXX=g++
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j6 --parallel 6

.\build\rscript.exe %* "Redscript - testing"