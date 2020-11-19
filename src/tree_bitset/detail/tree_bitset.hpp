#pragma once

template <typename BlockT>
TreeBitset<BlockT>::TreeBitset(const size_t exp_max)
{
  _max_elements = size_t{1} << exp_max;

  _num_element_blocks = std::max(size_t{1}, _max_elements >> bits_per_block_log2);

  _num_metadata_levels = static_cast<uint8_t>(math::int_log_ceil(bits_per_block, _max_elements));
  if(_num_metadata_levels)
    --_num_metadata_levels;

  const BlockT max_elements_mask =
    static_cast<BlockT>((size_t{1} << (_max_elements >> (_num_metadata_levels * bits_per_block_log2))) - 1);

  _num_metadata_blocks = 0;
  for(uint8_t metadata_lvl_idx = 0; metadata_lvl_idx < _num_metadata_levels; ++metadata_lvl_idx)
    _num_metadata_blocks += num_metadata_blocks_on_level(metadata_lvl_idx);

  _storage = std::make_unique<BlockT[]>(_num_element_blocks + _num_metadata_blocks);
  clean();
  // Unset root level bits for nonexisting elements if the tree isn't T-pyramid
  if(max_elements_mask)
    _storage[0] &= max_elements_mask;
}

template <typename BlockT>
inline uint8_t TreeBitset<BlockT>::num_metadata_levels() const
{
  return _num_metadata_levels;
}
template <typename BlockT>
inline size_t TreeBitset<BlockT>::num_element_blocks() const
{
  return _num_element_blocks;
}
template <typename BlockT>
inline size_t TreeBitset<BlockT>::num_metadata_blocks() const
{
  return _num_metadata_blocks;
}
template <typename BlockT>
inline size_t TreeBitset<BlockT>::max_elements() const
{
  return _max_elements;
}

template <typename BlockT>
inline size_t TreeBitset<BlockT>::num_metadata_blocks_on_level(const uint8_t level) const
{
  return size_t{1} << (bits_per_block_log2 * static_cast<size_t>(level));
}

template <typename BlockT>
void TreeBitset<BlockT>::clean()
{
  auto mem = _storage.get();
  std::fill(mem, mem + _num_element_blocks + _num_metadata_blocks, static_cast<BlockT>(~BlockT{0}));
}

template <typename BlockT>
inline bool TreeBitset<BlockT>::is_free(const size_t id) const
{
  const size_t block_idx   = id >> bits_per_block_log2;
  const size_t storage_idx = _num_metadata_blocks + block_idx;
  const size_t bit         = id & (bits_per_block - 1);
  return _storage[storage_idx] & (BlockT{1} << bit);
}

template <typename BlockT>
inline void TreeBitset<BlockT>::set_free(const size_t id, const bool value)
{
  const size_t block_idx   = id >> bits_per_block_log2;
  const size_t storage_idx = _num_metadata_blocks + block_idx;
  const size_t bit         = id & (bits_per_block - 1);

  bool should_update_metadata = false;
  if(value)
  {
    should_update_metadata = !_storage[storage_idx];
    _storage[storage_idx] |= BlockT{1} << bit;
  }
  else
  {
    _storage[storage_idx] &= ~(BlockT{1} << bit);
    should_update_metadata = !_storage[storage_idx];
  }
  if(should_update_metadata)
    update_metadata(id, value);
}

template <typename BlockT>
inline void TreeBitset<BlockT>::update_metadata(const size_t id, const bool all_bits_value)
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
      const bool need_updating_higher_tree_lvl_node = _storage[storage_idx] == BlockT{0};
      _storage[storage_idx] |= BlockT{1} << bit;
      if(!need_updating_higher_tree_lvl_node)
        break;
    }
    else
    {
      _storage[storage_idx] &= ~(BlockT{1} << bit);
      // We've obtained a bit on a block that still has some free(1) bits => no need to update higher lvls
      if(_storage[storage_idx] != BlockT{0})
        break;
    }
  }
}

template <typename BlockT>
size_t TreeBitset<BlockT>::obtain_id()
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
  _storage[storage_idx] &= ~(BlockT{1} << bit);

  const size_t id = bit + metadata_lvl_block_idx * bits_per_block;
  if(!_storage[storage_idx])
    update_metadata(id, false);

  return id;
}
