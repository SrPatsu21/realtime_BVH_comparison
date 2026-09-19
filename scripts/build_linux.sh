set -e

mkdir -p build
cd build
cmake .. -DCMAKE_CXX_FLAGS="-DUSE_BLAS_BSAH -DUSE_TLAS_BSAH" -DSHADER_DEFINITIONS="-DUSE_BLAS_BSAH -DUSE_TLAS_BSAH"
make -j$(nproc)