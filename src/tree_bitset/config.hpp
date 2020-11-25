#pragma once

// TOOD: handle dependencies properly via premake_scaffold ~_~
#include "../deps/meta-mate/src/meta_mate/config_builder.hpp"

enum class MaxIDPolicy {
  // this mainly impacts set_free(<id>, false) performance
  keep_max_id_current,
  on_demand_max_id_calc,
};

enum class FreeBitPolicy {
  //doesn't require memsetting on start, requires doing more bitwise NOT instructions
  zero,
  // default. Requires memsetting all data blocks to 1 on startup
  one
};

struct TreeBitsetPoliciesBuilder : mm::ConfigBuilder<MaxIDPolicy, FreeBitPolicy>
{
};

template <typename BlockT = std::uint64_t, typename Policies = TreeBitsetPoliciesBuilder::default_>
struct TreeBitsetConfig : public Policies
{
  using block_t = BlockT;
};

using DefaultTreeBitsetConfig = TreeBitsetConfig<>;
