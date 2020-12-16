#pragma once
#include <cassert>
#include <cstring>
#include "../tree_bitset.hpp"

namespace treebitset {
template <typename Config>
inline void TreeBitset<Config>::calculate_constants(const size_t exp_max)
{
  assert(exp_max < bits_per_block);

  _max_elements = size_t{1} << exp_max;

  _num_element_blocks = std::max(size_t{1}, _max_elements >> bits_per_block_log2);

  _num_metadata_levels = static_cast<uint8_t>(math::int_log_ceil(bits_per_block, _max_elements));
  if(_num_metadata_levels)
    --_num_metadata_levels;

  _num_metadata_blocks = 0;
  for(uint8_t metadata_lvl_idx = 0; metadata_lvl_idx < _num_metadata_levels; ++metadata_lvl_idx)
    _num_metadata_blocks += num_metadata_blocks_on_level(metadata_lvl_idx);
}

template <typename Config>
TreeBitset<Config>::TreeBitset(const size_t exp_max)
{
  calculate_constants(exp_max);
  _storage = std::make_unique<block_t[]>(_num_element_blocks + _num_metadata_blocks);

  clean();
  // Unset root level bits for nonexisting elements if the tree isn't T-pyramid
  const block_t max_elements_mask = max_element_mask();
  if(max_elements_mask)
    _storage[0] &= max_elements_mask;
}

template <typename Config>
inline typename TreeBitset<Config>::block_t TreeBitset<Config>::max_element_mask() const
{
  return static_cast<block_t>((size_t{1} << (_max_elements >> (_num_metadata_levels * bits_per_block_log2))) -
                              1);
}

template <typename Config>
inline uint8_t TreeBitset<Config>::num_metadata_levels() const
{
  return _num_metadata_levels;
}
template <typename Config>
inline size_t TreeBitset<Config>::num_element_blocks() const
{
  return _num_element_blocks;
}
template <typename Config>
inline size_t TreeBitset<Config>::num_metadata_blocks() const
{
  return _num_metadata_blocks;
}
template <typename Config>
inline size_t TreeBitset<Config>::max_elements() const
{
  return _max_elements;
}

template <typename Config>
inline size_t TreeBitset<Config>::max_used_id() const
{
  return _max_used_id;
}

template <typename Config>
inline size_t TreeBitset<Config>::num_metadata_blocks_on_level(const uint8_t level) const
{
  return size_t{1} << (bits_per_block_log2 * static_cast<size_t>(level));
}

template <typename Config>
void TreeBitset<Config>::clean()
{
  auto mem = _storage.get();
  std::fill(mem, mem + _num_element_blocks + _num_metadata_blocks, static_cast<block_t>(~block_t{0}));

  _max_used_id = invalid_id;
}

template <typename Config>
inline bool TreeBitset<Config>::is_free(const size_t id) const
{
  const size_t block_idx   = id >> bits_per_block_log2;
  const size_t storage_idx = _num_metadata_blocks + block_idx;
  const size_t bit         = id & (bits_per_block - 1);
  return _storage[storage_idx] & (block_t{1} << bit);
}

template <typename Config>
inline void TreeBitset<Config>::set_free(const size_t id, const bool value)
{
  const size_t block_idx   = id >> bits_per_block_log2;
  const size_t storage_idx = _num_metadata_blocks + block_idx;
  const size_t bit         = id & (bits_per_block - 1);


  bool should_update_metadata = false;
  if(value)
  {
    should_update_metadata = !_storage[storage_idx];
    _storage[storage_idx] |= block_t{1} << bit;
    if(id == _max_used_id)
      _max_used_id = find_new_smaller_max_used_id();
  }
  else
  {
    _max_used_id = _max_used_id == invalid_id ? id : std::max(_max_used_id, id);
    _storage[storage_idx] &= ~(block_t{1} << bit);
    should_update_metadata = !_storage[storage_idx];
  }
  if(should_update_metadata)
    update_metadata(id, value);
}

template <typename Config>
inline void TreeBitset<Config>::update_metadata(const size_t id, const bool all_bits_value)
{
  if(_num_metadata_levels == 0)
    return;
  size_t metadata_lvl_bit_offset  = id;
  size_t metadata_level_start_idx = _num_metadata_blocks;

  // Traverse the internal tree nodes upwards while updating metadata node values
  for(uint8_t lvl_idx = 0; lvl_idx < _num_metadata_levels; ++lvl_idx)
  {
    // Calculate bit and idx on the current level
    metadata_lvl_bit_offset >>= bits_per_block_log2;
    const size_t bit = metadata_lvl_bit_offset & (bits_per_block - 1);
    // Go to the start of the level
    metadata_level_start_idx -= num_metadata_blocks_on_level(_num_metadata_levels - lvl_idx - 1);

    const size_t metadata_lvl_block_idx = metadata_lvl_bit_offset >> bits_per_block_log2;
    const size_t storage_idx            = metadata_level_start_idx + metadata_lvl_block_idx;
    // Update metadata value for a corresponding node on the current lvl
    if(all_bits_value)
    {
      const bool need_updating_higher_tree_lvl_node = _storage[storage_idx] == block_t{0};
      _storage[storage_idx] |= block_t{1} << bit;
      if(!need_updating_higher_tree_lvl_node)
        break;
    }
    else
    {
      _storage[storage_idx] &= ~(block_t{1} << bit);
      // We've obtained a bit on a block that still has some free(1) bits => no need to update higher lvls
      if(_storage[storage_idx] != block_t{0})
        break;
    }
  }
}

template <typename Config>
inline size_t TreeBitset<Config>::find_new_smaller_max_used_id() const
{
  const block_t * const first_data_block = &_storage[num_metadata_blocks()];
  const size_t          initial_block =
    _max_used_id != invalid_id ? (_max_used_id >> bits_per_block_log2) : num_element_blocks() - 1;
  const block_t * previous_max_id_block = &_storage[_num_metadata_blocks + initial_block];
  // Traverse data blocks until we find the first block which doesnt contain only free elements
  while((previous_max_id_block != first_data_block) && (*previous_max_id_block == static_cast<block_t>(~0)))
    --previous_max_id_block;

  const size_t data_block_idx        = previous_max_id_block - first_data_block;
  const size_t first_id_of_max_block = data_block_idx * bits_per_block;

  block_t block_data = *previous_max_id_block;

  // A special case when we don't have any metadata levels and some of the bits are reserved.
  // TODO: that's really wasteful and reserved bits support should be supplied as a policy
  auto mask = max_element_mask();
  if(!_num_metadata_levels && mask)
    block_data |= ~mask;

  const size_t max_bit = bits_per_block - std::countl_one(block_data);
  return max_bit != 0 ? first_id_of_max_block + max_bit - 1 : invalid_id;
}

template <typename Config>
size_t TreeBitset<Config>::obtain_id()
{
  // Zero root node indicates that there're no free slots
  if(_storage[0] == 0)
    return invalid_id;

  size_t storage_idx            = 0;
  size_t metadata_lvl_block_idx = 0;
  // Traverse the internal tree nodes until we find a suitable element block
  for(uint8_t lvl_idx = 0; lvl_idx < _num_metadata_levels; ++lvl_idx)
  {
    // Calculate next level offset from the current lvl bit offset and the first non-zero bit
    metadata_lvl_block_idx = metadata_lvl_block_idx * bits_per_block +
                             std::countr_zero(_storage[storage_idx + metadata_lvl_block_idx]);
    // Go down to the next metadata level start
    storage_idx += num_metadata_blocks_on_level(lvl_idx);
  }
  storage_idx      = _num_metadata_blocks + metadata_lvl_block_idx;
  const size_t bit = std::countr_zero(_storage[storage_idx]);
  _storage[storage_idx] &= ~(block_t{1} << bit);

  const size_t id = bit + metadata_lvl_block_idx * bits_per_block;
  _max_used_id    = _max_used_id == invalid_id ? id : std::max(_max_used_id, id);
  if(!_storage[storage_idx])
    update_metadata(id, false);

  return id;
}

template <typename Config>
template <typename AddAbbreviationCallback, typename AddPackedBlockCallback>
inline void TreeBitset<Config>::pack(AddAbbreviationCallback abbrev_cb, AddPackedBlockCallback block_cb) const
{
  detail::rle_pack(_storage.get(), _num_element_blocks + _num_metadata_blocks, abbrev_cb, block_cb);
}

template <typename Config>
inline TreeBitset<Config> TreeBitset<Config>::unpack(const size_t               exp_max,
                                                     const block_t *            packed_blocks,
                                                     const RLEBitAbbreviation * abbreviations,
                                                     size_t                     abbreviations_count)
{
  TreeBitset result(exp_max);
  detail::rle_unpack(result._storage.get(),
                     result._num_element_blocks + result._num_metadata_blocks,
                     packed_blocks,
                     abbreviations,
                     abbreviations_count);
  if constexpr(Config::template get<MaxIDPolicy>() == MaxIDPolicy::keep_max_id_current)
  {
    result._max_used_id = result.find_new_smaller_max_used_id();
  }
  return result;
}

template <typename Config>
inline bool operator==(const TreeBitset<Config> & lhs, const TreeBitset<Config> & rhs)
{
  if(lhs._max_elements != rhs._max_elements)
    return false;

  if constexpr(Config::template get<MaxIDPolicy>() == MaxIDPolicy::keep_max_id_current)
  {
    if(lhs._max_used_id != rhs._max_used_id)
      return false;
  }
  const size_t storage_bytes =
    (lhs._num_element_blocks + lhs._num_metadata_blocks) * sizeof(typename Config::block_t);
  return !memcmp(lhs._storage.get(), rhs._storage.get(), storage_bytes);
}

template <typename Config>
inline bool operator!=(const TreeBitset<Config> & lhs, const TreeBitset<Config> & rhs)
{
  return !(lhs == rhs);
}

template <typename Config>
class TreeBitset<Config>::IDIterator
{
  block_t   _block_mask = static_cast<block_t>(~block_t{0});
  block_t * _ptr        = nullptr;
  block_t * _start_ptr  = nullptr;
  block_t * _end_ptr    = nullptr;

  inline void advanced_to_next_block()
  {
    // Advance the _ptr to obtain the first used ID
    while(_ptr != _end_ptr && (*_ptr == static_cast<block_t>(~block_t{0})))
      ++_ptr;
  }

  inline void advance()
  {
    const block_t reversed_block = ~(*_ptr);
    const int     bit_idx        = std::countr_zero(static_cast<block_t>(reversed_block & _block_mask));
    _block_mask                  = _block_mask & ~(block_t{1} << bit_idx);
    if(static_cast<block_t>(reversed_block & _block_mask) == block_t{0})
    {
      _block_mask = static_cast<block_t>(~block_t{0});
      ++_ptr;
      advanced_to_next_block();
    }
  }

  inline size_t current_id() const
  {
    const size_t  block_offset     = TreeBitset<Config>::bits_per_block * (_ptr - _start_ptr);
    const block_t reversed_block   = ~(*_ptr);
    const size_t  block_bit_offset = std::countr_zero(static_cast<block_t>(reversed_block & _block_mask));
    return block_offset + block_bit_offset;
  }

public:
  using iterator_category = std::forward_iterator_tag;
  using value_type        = size_t;
  using difference_type   = std::ptrdiff_t;
  using pointer           = size_t *;
  using reference         = size_t;

  IDIterator(const TreeBitset<Config> & container)
    : _ptr{&container._storage[container.num_metadata_blocks()]}, _start_ptr{_ptr}
  {
    const size_t max_used_id = container.max_used_id();
    if(max_used_id != invalid_id)
      _end_ptr = &container._storage[container.num_metadata_blocks() + (max_used_id >> bits_per_block_log2)];
    else
      _end_ptr = _start_ptr;
    advanced_to_next_block();
  }

  IDIterator operator++()
  {
    IDIterator i = *this;
    advance();
    return i;
  }
  IDIterator operator++(int)
  {
    advance();
    return *this;
  }
  value_type operator*() { return current_id(); }
  bool       operator==(const IDIterator & rhs) { return _ptr == rhs._ptr && _block_mask == rhs._block_mask; }
  bool       operator!=(const IDIterator & rhs) { return _ptr != rhs._ptr || _block_mask != rhs._block_mask; }

  IDIterator begin() const { return *this; }
  IDIterator end() const
  {
    IDIterator end = *this;
    end._ptr       = end._end_ptr;
    return end;
  }
};

template <typename Config>
inline typename TreeBitset<Config>::IDIterator TreeBitset<Config>::used_ids_iter() const
{
  return *this;
}
}