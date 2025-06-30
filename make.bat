set CXX=g++
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j

.\build\rscript.exe %* "Redscript - testing"