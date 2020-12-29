#pragma once
#include <limits>
#include <cinttypes>
#include <memory>
#include <algorithm>

#include "detail/bit"
#include "detail/math_utils.hpp"
#include "detail/bit_rle_pack.hpp"

#include "config.hpp"

#undef max

namespace treebitset {
template <typename Config = DefaultTreeBitsetConfig>
class TreeBitset : Config
{
  class IDIterator;

public:
  using block_t = typename Config::block_t;
  static_assert(sizeof(block_t) > 1, "block size must be bigger than 1 byte!");

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

  template <typename AddAbbreviationCallback, typename AddPackedBlockCallback>
  void pack(AddAbbreviationCallback abbrev_cb, AddPackedBlockCallback block_cb) const;

  static TreeBitset unpack(const size_t               exp_max,
                           const block_t *            packed_blocks,
                           const RLEBitAbbreviation * abbreviations,
                           size_t                     abbreviations_count);

  template <typename C>
  friend inline bool operator==(const TreeBitset<C> & lhs, const TreeBitset<C> & rhs);

  template <typename C>
  friend inline bool operator!=(const TreeBitset<C> & lhs, const TreeBitset<C> & rhs);

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

  inline void    calculate_constants(const size_t exp_max);
  inline block_t max_element_mask() const;
  inline size_t  num_metadata_blocks_on_level(const uint8_t level) const;
  inline void    update_metadata(const size_t id, const bool all_bits_value);
  inline size_t  find_new_smaller_max_used_id() const;
};
}
#include "detail/tree_bitset.hpp"
