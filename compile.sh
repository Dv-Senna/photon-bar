export CC=clang
export CXX=clang++
cmake . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=On -G "Ninja"
cmake --build build
