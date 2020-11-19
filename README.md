# Overview
TreeBitset is a fixed-size bitset with fast (logarithmic) free/occupied bit lookup. It's best suited as a building block for implementing stuff like database page-caches, ECS ID dispensers, serialization systems etc.

## Goals
- low overhead
- no allocations
- configurable via templates

## Highlights
- number of elements is restricted to only powers of 2 to improve performance
- lookup complexity is `log[sizeof(node_size_t), max_elements]`
- fast lookup works by maintaining a B-tree-like structure with metadata nodes
- node size is configurable via a template parameter
- metadata tree is laid down as an implicit data structure in memory

## Inner structure
Let's consider `TreeBitset<uint8_t>` with 128 elements. It'll have 2 metadata levels:

```
1st  level: uint8_t
2nd  level: uint8_t x8
data level: uint8_t x16
```
`Nth` bit in a metadata level `M` indicates whether there are any free bits in the `M+1` level. All levels are laid out one after another in a contiguous memory block.

# TODO
- benchmarks
- implement id list transforming into ranges for set_free_for_range
- implement set_free_for_range
- external memory block supply policy
- zero/one for free-indication policy
- template policy for crab-spinlocking