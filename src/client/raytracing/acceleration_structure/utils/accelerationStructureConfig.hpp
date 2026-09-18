#pragma once

#include "../cpu/builder/BVHBuilder.hpp"
#include "../cpu/builder/BVH8Builder.hpp"
#include "../cpu/builder/BinnedSAHBuilder.hpp"

#include "../cpu/node/BVHNode.hpp"
#include "../cpu/node/BVH8Node.hpp"

//*======================
//* BLAS
//*======================

#if defined(USE_BLAS_BVH8)

    using DefaultBLASNode = BVH8Node;
    using DefaultBLASBuilder = BVH8Builder<DefaultBLASNode>;

#elif defined(USE_BLAS_BSAH)

    using DefaultBLASNode = BVHNode;
    using DefaultBLASBuilder = BinnedSAHBuilder<DefaultBLASNode>;

#else

    using DefaultBLASNode = BVHNode;
    using DefaultBLASBuilder = BVHBuilder<DefaultBLASNode>;

#endif

//*======================
//* TLAS
//*======================

#if defined(USE_TLAS_BVH8)

    using DefaultTLASNode = BVH8Node;
    using DefaultTLASBuilder = BVH8Builder<DefaultTLASNode>;

#elif defined(USE_TLAS_BSAH)

    using DefaultTLASNode = BVHNode;
    using DefaultTLASBuilder = BinnedSAHBuilder<DefaultTLASNode>;

#else

    using DefaultTLASNode = BVHNode;
    using DefaultTLASBuilder = BVHBuilder<DefaultTLASNode>;

#endif