set -e

mkdir -p build
cd build
cmake .. -DCMAKE_CXX_FLAGS="-DUSE_BLAS_LBVH -DUSE_TLAS_LBVH" -DSHADER_DEFINITIONS="-DUSE_BLAS_LBVH -DUSE_TLAS_LBVH"
make -j$(nproc)