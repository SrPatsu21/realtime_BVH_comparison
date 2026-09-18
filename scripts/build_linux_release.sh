set -e

mkdir -p build-release
cd build-release
cmake .. \
    -DCMAKE_CXX_FLAGS="-DUSE_BLAS_BVH8 -DUSE_TLAS_BVH8" \
    -DSHADER_DEFINITIONS="-DUSE_BLAS_BVH8 -DUSE_TLAS_BVH8"
make -j$(nproc)
