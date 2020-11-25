#pragma once
#include <limits>
#include <cinttypes>
#include <memory>

#include "detail/bit"
#include "detail/math_utils.hpp"

#include "config.hpp"

template <typename Config = DefaultTreeBitsetConfig>
class TreeBitset : Config
{
  class IDIterator;

public:
  using block_t                                 = typename Config::block_t;
  constexpr static inline size_t invalid_id     = std::numeric_limits<size_t>::max();
  constexpr static inline size_t bits_per_block = std::numeric_limits<block_t>::digits;

  // Tree bitset will have a capacity for 2^exp_max elements
  TreeBitset(const size_t exp_max);

  // Get value of bit id
  inline bool is_free(const size_t id) const;
  // Set bit id value
  inline void set_free(const size_t id, const bool free);
  // Bulk-set many bit values
  void set_free_for_range(const size_t min_id, const size_t max_id, const bool value);

  // Find the first free bit id, unset it and get the id
  size_t obtain_id();

  // Free all ids
  void clean();

  IDIterator used_ids_iter() const;

  inline size_t max_used_id() const;

  inline uint8_t num_metadata_levels() const;
  inline size_t  num_element_blocks() const;
  inline size_t  num_metadata_blocks() const;
  inline size_t  max_elements() const;

private:
  friend class IDIterator;

  constexpr static inline size_t bits_per_block_log2 = math::int_log2(bits_per_block);

  std::unique_ptr<block_t[]> _storage;
  size_t                     _max_used_id = invalid_id;

  // These fields are constant throughout object lifetime
  size_t  _num_metadata_blocks;
  size_t  _num_element_blocks;
  uint8_t _num_metadata_levels;
  size_t  _max_elements;

  inline block_t max_element_mask() const;
  inline size_t  num_metadata_blocks_on_level(const uint8_t level) const;
  inline void    update_metadata(const size_t id, const bool all_bits_value);
  inline size_t  find_new_smaller_max_used_id() const;
};

#include "detail/tree_bitset.hpp"