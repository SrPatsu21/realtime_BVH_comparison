#include "builder/BVHBuilder.hpp"
#include "BVHNode.hpp"

//*======================
//* BLAS
//*======================

#ifdef USE_BLAS_SAH_NODE
    using DefaultBLASNode = SAHNode;
#else
    using DefaultBLASNode = BVHNode;
#endif

#ifdef USE_BLAS_SAH_BUILDER
    using DefaultBLASBuilder = SAHBuilder<DefaultBLASNode>;
#else
    using DefaultBLASBuilder = BVHBuilder<DefaultBLASNode>;
#endif

//*======================
//* TLAS
//*======================

#ifdef USE_TLAS_SAH_NODE
    using DefaultTLASNode = SAHNode;
#else
    using DefaultTLASNode = BVHNode;
#endif

#ifdef USE_TLAS_SAH_BUILDER
    using DefaultTLASBuilder = SAHBuilder<DefaultTLASNode>;
#else
    using DefaultTLASBuilder = BVHBuilder<DefaultTLASNode>;
#endif