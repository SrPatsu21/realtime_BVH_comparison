# BVH Implementation Plan

## Goal

Add three new BVH construction/traversal variants to the acceleration-structure system, in addition to the existing binary BVH:

1. **LBVH** — Linear Bounding Volume Hierarchy
2. **BVH8** — 8-wide Bounding Volume Hierarchy
3. **Binned SAH (BSAH)** — Binned Surface Area Heuristic

The implementation should keep the BLAS/TLAS management code independent from the concrete BVH construction method as much as possible.

---

# 1. Common Architecture

The existing architecture already separates the acceleration-structure manager from the BVH builder through:

- Node type
- Builder type
- BLAS/TLAS instance construction

The new implementations should preserve this separation.

Each BVH method should provide a node type and a builder compatible with the corresponding `BLASInstanceBuilder` or `TLASInstanceBuilder`.

Conceptually:

```text
AccelerationStructureManager
        |
        +-- BLASInstanceBuilder
        |       |
        |       +-- selected BLAS Node
        |       +-- selected BLAS Builder
        |
        +-- TLASInstanceBuilder
                |
                +-- selected TLAS Node
                +-- selected TLAS Builder
```

The configuration header should select the implementation at compile time.

---

# 2. Existing Binary BVH

The current implementation is the baseline.

Characteristics:

- Binary tree
- Each internal node has up to two children
- Primitives are recursively partitioned
- Existing `BVHNode`
- Existing `BVHBuilder`

This implementation should remain available as the default configuration.

The other methods should be implemented without breaking the existing binary BVH.

---

# 3. LBVH

## 3.1 Overview

LBVH (Linear Bounding Volume Hierarchy) builds a hierarchy using Morton codes generated from primitive centroids.

The general pipeline is:

```text
Primitives
    |
    v
Compute centroids
    |
    v
Normalize centroid positions
    |
    v
Generate Morton codes
    |
    v
Sort primitives by Morton code
    |
    v
Generate hierarchy from Morton-code differences
    |
    v
Compute node bounds
```

The Morton code maps a 3D position into a linear integer representation by interleaving the binary bits of the spatial coordinates.

Nearby spatial positions tend to produce nearby Morton codes, providing a spatial ordering suitable for constructing a hierarchy.

## 3.2 Initial implementation

The first LBVH implementation should focus on correctness rather than maximum parallel performance.

Suggested stages:

1. Calculate primitive centroids.
2. Calculate the centroid bounding box.
3. Normalize centroids into a unit cube.
4. Generate Morton codes.
5. Sort `(MortonCode, PrimitiveRef)` pairs.
6. Construct the binary hierarchy from the sorted Morton codes.
7. Calculate bounds for internal nodes.
8. Produce the final node array compatible with the rest of the acceleration-structure system.

## 3.3 Hierarchy construction

The hierarchy can be constructed by examining the common prefix / differing bits between neighboring Morton codes.

The key operation is determining the highest bit position at which the Morton codes differ. This information determines where ranges of primitives should be split.

A later optimization phase may replace the initial implementation with a more parallel construction strategy.

## 3.4 Expected characteristics

Advantages:

- Very fast construction
- Suitable for highly parallel construction
- Simple primitive ordering mechanism
- Good candidate for dynamic scenes

Disadvantages:

- Usually lower-quality trees than a well-built SAH hierarchy
- Potentially worse ray traversal performance
- Requires Morton-code generation and sorting
- Requires temporary buffers

---

# 4. BVH8

## 4.1 Overview

BVH8 is a wide BVH in which an internal node can contain up to eight children instead of two.

The main objective is to reduce tree depth and therefore reduce traversal steps.

Conceptually:

```text
Binary BVH:

        Root
       /    \
      A      B
     / \    / \

BVH8:

             Root
      / / / / | | | \
     A B C D  E F G H
```

The exact node representation should be designed around efficient GPU traversal and memory access.

## 4.2 Node design

A BVH8 node should contain enough information to represent up to eight children.

A possible conceptual representation is:

```cpp
struct BVH8Node
{
    AABB bounds[8];
    uint32_t children[8];

    uint32_t childCount;
    bool leaf;
};
```

The final representation should be optimized before being considered stable.

Potential optimizations include:

- Separate arrays for child bounds and child indices
- Compact child representation
- Explicit leaf ranges
- GPU-friendly alignment
- Fixed-size storage for eight children
- Compressed bounds

## 4.3 Construction strategy

The first implementation should prioritize a reliable construction algorithm.

A practical initial strategy is:

1. Build a high-quality binary BVH.
2. Convert or collapse groups of binary nodes into up to eight children.
3. Produce the final BVH8 representation.
4. Validate that the resulting hierarchy preserves the original bounds and primitive coverage.

This allows BVH8 traversal to be developed independently from a completely new BVH8 construction algorithm.

A later implementation can directly construct an 8-wide hierarchy if that provides measurable benefits.

## 4.4 Traversal considerations

BVH8 changes the traversal algorithm substantially.

Instead of:

```text
intersect node
    |
    +-- child 0
    +-- child 1
```

the traversal must test up to eight child AABBs and determine the order in which they should be visited.

Important considerations:

- Efficient AABB intersection
- Sorting or selecting children by ray distance
- Stack usage
- GPU memory layout
- Divergence between rays
- Cache behavior

## 4.5 Expected characteristics

Advantages:

- Shallower hierarchy
- Potentially fewer traversal steps
- Better suited to SIMD/SIMT traversal
- Potentially improved memory locality

Disadvantages:

- Larger node representation
- More AABB tests per visited node
- More complex traversal
- Potentially higher memory bandwidth requirements

---

# 5. Binned SAH (BSAH)

## 5.1 Terminology

The appropriate name for this method is **Binned SAH** or **Binned Surface Area Heuristic**.

"BSAH BVH" is understandable, but BSAH describes the construction heuristic rather than a fundamentally different BVH representation.

The resulting structure can still be a normal binary BVH.

Therefore the implementation should preferably use names such as:

```text
BinnedSAHBuilder
BinnedSAHNode
```

or, if the node layout remains identical to the normal binary BVH:

```text
BVHNode
BinnedSAHBuilder
```

## 5.2 Overview

Binned SAH approximates the full Surface Area Heuristic while significantly reducing construction cost.

Instead of evaluating every possible primitive split, the centroid or spatial extent is divided into a fixed number of bins.

Conceptually:

```text
Primitive centroids
        |
        v
Select split axis
        |
        v
Create N bins
        |
        v
Assign primitives to bins
        |
        v
Evaluate splits between bins
        |
        v
Select minimum SAH cost
        |
        v
Partition primitives
        |
        v
Recurse
```

## 5.3 Initial implementation

The first implementation should use a fixed number of bins, for example:

```cpp
static constexpr uint32_t BIN_COUNT = 16;
```

The exact number should remain configurable so that different values can be benchmarked.

For each candidate axis:

1. Compute the centroid range.
2. Assign primitives to bins.
3. Compute prefix bounds/counts.
4. Compute suffix bounds/counts.
5. Evaluate the SAH cost for each possible split.
6. Select the lowest-cost split.
7. Partition the primitives.
8. Recurse.

## 5.4 SAH cost

For a candidate split, the cost can be evaluated from the surface areas of the resulting child bounds and their primitive counts.

Conceptually:

```text
Cost =
    Area(left)  * Count(left)
  + Area(right) * Count(right)
```

The exact normalization and traversal/intersection costs should be kept consistent with the existing BVH implementation.

## 5.5 Empty and degenerate splits

The implementation must explicitly handle:

- Empty centroid ranges
- All centroids sharing the same coordinate
- Empty bins
- Splits that produce zero primitives on one side
- Very small primitive ranges
- Numerical precision issues

If no valid SAH split can be found, the builder should fall back to a robust partition strategy.

## 5.6 Expected characteristics

Advantages:

- Better construction quality than simple spatial splitting
- Lower construction cost than evaluating every primitive split
- Usually good ray traversal performance
- Well suited to static or moderately dynamic geometry

Disadvantages:

- More expensive to build than a simple binary BVH
- Usually slower to construct than LBVH
- Requires temporary bin data
- Quality depends on the number of bins

---

# 6. Relationship Between the Three Methods

These methods should not be treated as three completely unrelated BVH representations.

There are two different concepts:

### Construction algorithm

Examples:

- Standard recursive BVH construction
- LBVH
- Binned SAH

### Node topology / representation

Examples:

- Binary BVH
- BVH4
- BVH8
- BVH16

Therefore:

```text
LBVH
    -> primarily a construction algorithm

Binned SAH
    -> primarily a construction heuristic

BVH8
    -> primarily a wide node representation / topology
```

This distinction should be preserved in the architecture.

For example, a future implementation could potentially combine:

```text
LBVH + BVH8
Binned SAH + BVH8
```

without requiring a completely new acceleration-structure manager.

---

# 7. Configuration Plan

The configuration should expose separate choices for BLAS and TLAS.

For example:

```cpp
#define BLAS_BVH
#define TLAS_BVH
```

or equivalent compile-time selection macros.

The configuration should map each selection to:

```text
NodeType
BuilderType
```

Example:

```text
BLAS_BVH
    -> BVHNode
    -> BVHBuilder<BVHNode>

BLAS_LBVH
    -> LBVHNode
    -> LBVHBuilder<LBVHNode>

BLAS_BVH8
    -> BVH8Node
    -> BVH8Builder<BVH8Node>

BLAS_BSAH
    -> BVHNode
    -> BinnedSAHBuilder<BVHNode>
```

The same concept applies independently to TLAS.

---

# 8. BLAS/TLAS Considerations

The same construction method does not necessarily need to be used for BLAS and TLAS.

The configuration should therefore allow combinations such as:

```text
BLAS: Binned SAH
TLAS: LBVH
```

or:

```text
BLAS: BVH8
TLAS: BVH8
```

This is useful because BLAS and TLAS have different characteristics.

BLAS generally contains triangle geometry and may benefit from a higher-quality construction.

TLAS contains instances and may prioritize construction speed, especially when instances move frequently.

---

# 9. Implementation Order

The recommended implementation order is:

## Phase 1 — Configuration

Implement the compile-time selection system.

Requirements:

- Keep the current binary BVH as the default.
- Add explicit selections for LBVH, BVH8 and Binned SAH.
- Keep BLAS and TLAS selections independent.
- Expose `DefaultBLASNode`, `DefaultBLASBuilder`, `DefaultTLASNode` and `DefaultTLASBuilder`.

## Phase 2 — Builder Interface

Adapt:

```text
BLASInstanceBuilder
TLASInstanceBuilder
```

so they no longer directly depend on:

```text
BVHNode
BVHBuilder
```

Instead, they should use the selected configuration types.

## Phase 3 — Binned SAH

Implement Binned SAH first because it can reuse much of the existing binary BVH infrastructure.

Validate:

- Node bounds
- Primitive coverage
- Leaf generation
- BLAS instance generation
- TLAS instance generation
- Traversal correctness

## Phase 4 — LBVH

Implement:

- Centroid bounds
- Morton encoding
- Morton sorting
- Hierarchy generation
- Node bounds
- Leaf generation

Then compare construction time and traversal performance against the existing BVH and Binned SAH.

## Phase 5 — BVH8

Implement:

- BVH8 node representation
- Binary-to-BVH8 conversion or direct BVH8 construction
- BVH8 GPU layout
- BVH8 traversal
- BLAS/TLAS integration

This phase should include memory-layout and traversal benchmarks.

---

# 10. Validation

Every implementation should first be validated against the same set of invariants.

### Structural validation

Check that:

- Every primitive appears exactly where expected.
- Every leaf references a valid primitive range.
- Every internal node references valid children.
- Every node AABB contains all geometry below it.
- No invalid indices are generated.
- Root bounds contain the complete structure.

### BLAS validation

Compare:

```text
Triangle count
Leaf count
Node count
Tree depth
Build time
Memory usage
```

### TLAS validation

Compare:

```text
Instance count
Leaf count
Node count
Tree depth
Build time
Memory usage
```

### Traversal validation

For identical rays and scene data:

```text
Binary BVH
Binned SAH
LBVH
BVH8
```

must produce equivalent intersection results.

Only after correctness is established should performance optimization begin.

---

# 11. Benchmark Plan

The implementations should be compared using the same scenes and workloads.

Measure:

```text
Build time
Node count
Memory consumption
Average tree depth
Maximum tree depth
Ray traversal time
Total frame time
```

For dynamic scenes, also measure rebuild/update cost.

The primary comparison should eventually answer:

```text
Construction speed
        vs
BVH quality
        vs
Traversal performance
        vs
Memory usage
```

This will make it possible to determine which method is most appropriate for BLAS and TLAS independently.

---

# 12. Future Extensions

The architecture should leave room for:

- HLBVH
- SBVH
- LBVH + SAH hybrid construction
- Binned SAH + spatial splits
- BVH4
- BVH16
- Compressed BVH nodes
- GPU-specific node layouts
- Parallel GPU construction

These should not be implemented as part of the initial three-method milestone unless benchmarking shows a clear need.

---

# Initial Target

The first milestone is therefore:

```text
Existing BVH
    +
Binned SAH
    +
LBVH
    +
BVH8
```

with independent compile-time selection for:

```text
BLAS
TLAS
```

while keeping the acceleration-structure manager independent from the specific construction algorithm.
