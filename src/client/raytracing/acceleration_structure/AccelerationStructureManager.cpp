#include "AccelerationStructureManager.hpp"

template<
    typename TLBuilderType,
    typename BLBuilderType
>
const AccelerationStructure<
    typename AccelerationStructureManager<
        TLBuilderType,
        BLBuilderType
    >::TLNodeType
>&
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::getTLAS(
) const
{
    return tlas;
}