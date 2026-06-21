set -e

mkdir -p build-windows-release
cd build-windows-release
cmake .. -D CMAKE_TOOLCHAIN_FILE=../toolchain-mingw.cmake -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)