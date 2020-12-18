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
init + obtain all in order                     100             1     6.16872 s
                                        68.0077 ms    66.4381 ms    70.6069 ms
                                        10.1123 ms    7.16329 ms    16.1869 ms
                                                                              
obtain 1024 in order                           100            16     4.9232 ms
                                        8.16769 us    7.77831 us    8.72425 us
                                        2.34166 us    1.81383 us    3.45708 us
                                                                              
obtain all in order                            100             1     1.80206 s
                                        62.1374 ms    61.3321 ms    63.4338 ms
                                        5.11557 ms    3.63539 ms    7.70246 ms
                                                                              
obtain half in order                           100             1    818.701 ms
                                        31.1191 ms    30.7931 ms    31.5254 ms
                                        1.85207 ms    1.54352 ms    2.29246 ms
                                                                              
obtain half - random free order                100             1    791.221 ms
                                          31.07 ms    30.7271 ms     31.551 ms
                                         2.0571 ms     1.5805 ms    2.69675 ms
                                                                              
set all unfree manually                        100             1     5.51596 s
                                        26.6275 ms    26.4264 ms    26.8713 ms
                                        1.12454 ms    937.962 us    1.41785 ms
                                                                              
pack+unpack to a pre-allocated                                                
buffers                                        100             1      1.2609 s
                                        12.8795 ms     12.734 ms     13.066 ms
                                        835.339 us    690.031 us    1.14154 ms
```

# TODO
- <s>benchmarks</s>
- <s>support packing</s>
- zero/one for free-indication policy

- use factory with error-handling
- external memory block supply policy

- make IDiterator bidirectional_iterator_tag
- support building metadata from a provided block

- implement set_free_for_range
- implement id list transforming into ranges for set_free_for_range

- AVX
