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
`Nth` bit in a metadata level `M` indicates whether there are any free bits in the `M+1` level `Nth` block. All levels are laid out one after another in a contiguous memory block.

## Benchmark results
CPU: i7-6700

TreeBitset<uint64> with 2^23 elements:

```
benchmark name                       samples       iterations    estimated    
                                     mean          low mean      high mean    
                                     std dev       low std dev   high std dev 
------------------------------------------------------------------------------
init + obtain all in order                     100             1     6.47031 s
                                        60.1453 ms    59.6745 ms     60.779 ms
                                        2.76415 ms    2.21249 ms    4.01089 ms
                                                                              
obtain 1024 in order                           100            26     5.1376 ms
                                        7.42546 us    7.17762 us    7.79862 us
                                        1.53341 us    1.15055 us    2.27446 us
                                                                              
obtain all in order                            100             1     1.91044 s
                                        67.5105 ms    66.2268 ms    68.9223 ms
                                         6.8658 ms    6.08147 ms    7.94221 ms
                                                                              
obtain half in order                           100             1    924.036 ms
                                        32.8601 ms    32.3722 ms    33.4521 ms
                                        2.73764 ms    2.32745 ms    3.26431 ms
                                                                              
obtain half - random free order                100             1    894.213 ms
                                        31.4904 ms    31.0066 ms    32.1196 ms
                                        2.80062 ms    2.29626 ms    3.60343 ms
                                                                              
set all unfree manually                        100             1     4.88769 s
                                        18.3903 ms     18.178 ms    18.6825 ms
                                        1.26279 ms    971.006 us    1.69012 ms
```

# TODO
- <s>benchmarks</s>
- implement id list transforming into ranges for set_free_for_range
- support building metadata from a provided block
- use factory with error-handling
- support packing
- implement set_free_for_range
- external memory block supply policy
- zero/one for free-indication policy
- make IDiterator bidirectional_iterator_tag