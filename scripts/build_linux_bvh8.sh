set -e

mkdir -p build
cd build
cmake .. -DCMAKE_CXX_FLAGS="-DUSE_BLAS_BVH8 -DUSE_TLAS_BVH8" -DSHADER_DEFINITIONS="-DUSE_BLAS_BVH8 -DUSE_TLAS_BVH8"
make -j$(nproc)