#pragma once

// run-length encoding which encodes abbreviations as a pair of u64s: [pos, length] and stores
// encoded block value as a highest bit of pos.

#include <cinttypes>
#include <cstddef>

namespace treebitset {
struct RLEBitAbbreviation
{
  uint64_t position_and_val = 0;
  uint64_t nblocks          = 0;
};

namespace detail {

template <typename block_t, typename AddAbbreviationCallback, typename AddPackedBlockCallback>
void rle_pack(const block_t *         blocks,
              const size_t            nBlocks,
              AddAbbreviationCallback abbrev_cb,
              AddPackedBlockCallback  block_cb)
{
  block_t            block_value = blocks[0];
  RLEBitAbbreviation next_abbreviation{};
  next_abbreviation.nblocks = 1;

  auto add_abbr = [&](const size_t i, const bool abbr_value, const uint64_t block_start) -> bool {
    const uint64_t nreps = i - block_start;
    if(sizeof(block_t) * nreps > sizeof(RLEBitAbbreviation))
    {
      next_abbreviation.position_and_val = block_start;
      next_abbreviation.position_and_val |= uint64_t{abbr_value} << (sizeof(uint64_t) * 8 - 1);
      next_abbreviation.nblocks = nreps;
      abbrev_cb(next_abbreviation);
      return true;
    }
    // no reason to add an abbreviation if we don't save any memory
    return false;
  };

  auto is_bit_packable = [](const block_t val) {
    const bool all_zeroes = !val;
    const bool all_ones   = val == static_cast<block_t>(~block_t{0});
    return all_zeroes || all_ones;
  };

  uint64_t block_start = 0;
  for(size_t i = 1; i < nBlocks; ++i)
  {
    const block_t val = blocks[i];
    if(val != block_value)
    {
      bool added = false;
      if(is_bit_packable(block_value))
        added = add_abbr(i, !!block_value, block_start);
      if(!added)
      {
        for(size_t idx = block_start; idx < i; ++idx)
          block_cb(blocks[idx]);
      }
      block_start = i;
      block_value = val;
    }
  }

  if(!is_bit_packable(block_value) || !add_abbr(nBlocks, !!block_value, block_start))
  {
    for(size_t idx = block_start; idx < nBlocks; ++idx)
      block_cb(blocks[idx]);
  }
}

template <typename block_t>
void rle_unpack(block_t *                  unpacked_blocks,
                const size_t               unpacked_block_count,
                const block_t *            packed_blocks,
                const RLEBitAbbreviation * abbreviations,
                size_t                     abbreviations_count)
{
  size_t packed_idx   = 0;
  size_t unpacked_idx = 0;
  for(size_t aidx = 0; aidx < abbreviations_count; ++aidx)
  {
    const auto &   abbr     = abbreviations[aidx];
    const bool     val      = abbr.position_and_val & (uint64_t{1} << (sizeof(uint64_t) * 8 - 1));
    const uint64_t position = abbr.position_and_val & ~(uint64_t{1} << (sizeof(uint64_t) * 8 - 1));

    while(unpacked_idx < position)
      unpacked_blocks[unpacked_idx++] = packed_blocks[packed_idx++];
    for(size_t i = 0; i < abbr.nblocks; ++i)
      unpacked_blocks[unpacked_idx++] = val ? static_cast<block_t>(~block_t{0}) : 0;
  }
  while(unpacked_idx < unpacked_block_count)
    unpacked_blocks[unpacked_idx++] = packed_blocks[packed_idx++];
}

}
}