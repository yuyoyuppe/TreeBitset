#include <tree_bitset/tree_bitset.hpp>

#include <array>
#include <vector>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <tuple>

#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "catch_amalgamated.hpp"

using namespace treebitset;

auto get_seed()
{
  std::random_device::result_type seed = std::random_device{}();
  //seed                                 = 451341145;
  printf("seed: %u\n", seed);
  return seed;
}
std::mt19937 g{get_seed()};

const std::array max_elements_exp_vals = {6, 7, 12, 13};

template <typename BlockT = std::uint64_t>
inline std::tuple<TreeBitset<TreeBitsetConfig<BlockT>>, std::vector<bool>> prepare_random_data(
  const size_t num_elements_exp, const size_t max_elements_divider)
{
  auto result = std::make_tuple(TreeBitset<TreeBitsetConfig<BlockT>>{num_elements_exp},
                                std::vector<bool>(size_t{1} << num_elements_exp, true));

  auto & tb     = std::get<0>(result);
  auto & bitset = std::get<1>(result);

  const size_t max_elements = tb.max_elements();
  INFO("max elements = " << max_elements);
  for(size_t idx = 0; idx < max_elements / max_elements_divider; ++idx)
  {
    const size_t selected_idx = g() & (max_elements - 1);
    const bool   set          = g() & 1;
    tb.set_free(selected_idx, set);
    bitset[selected_idx] = set;
  }
  return result;
}

TEST_CASE("Tree configuration", "[properties]")
{
  SECTION("TreeBitset<uint64_t> with 1 element")
  {
    TreeBitset tb{0};
    REQUIRE(tb.num_metadata_levels() == 0);
    REQUIRE(tb.num_element_blocks() == 1);
    REQUIRE(tb.num_metadata_blocks() == 0);
    REQUIRE(tb.max_elements() == 1);
  }
  SECTION("TreeBitset<uint64_t> with 64 elements")
  {
    TreeBitset tb{6};
    REQUIRE(tb.num_metadata_levels() == 0);
    REQUIRE(tb.num_element_blocks() == 1);
    REQUIRE(tb.num_metadata_blocks() == 0);
    REQUIRE(tb.max_elements() == 64);
  }
  SECTION("TreeBitset<uint64_t> with 2^12 elements")
  {
    TreeBitset tb{12};
    REQUIRE(tb.num_metadata_levels() == 1);
    REQUIRE(tb.num_element_blocks() == 64);
    REQUIRE(tb.num_metadata_blocks() == 1);
    REQUIRE(tb.max_elements() == 64 * 64);
  }

  SECTION("TreeBitset<uint64_t> with 2^13 elements")
  {
    TreeBitset tb{13};
    REQUIRE(tb.num_metadata_levels() == 2);
    REQUIRE(tb.num_element_blocks() == 128);
    REQUIRE(tb.num_metadata_blocks() == 1 + 64);
    REQUIRE(tb.max_elements() == 64 * 64 * 2);
  }

  // we don't support uint8_t for now, since it doesn't support all max_elements_exp_vals ^_~. need to
  // accomodate to it
  //SECTION("TreeBitset<uint8_t> with 2^7 elements")
  //{
  //  TreeBitset<TreeBitsetConfig<std::uint8_t>> tb{7};
  //  REQUIRE(tb.num_metadata_levels() == 2);
  //  REQUIRE(tb.num_element_blocks() == 16);
  //  REQUIRE(tb.num_metadata_blocks() == 1 + 8);
  //  REQUIRE(tb.max_elements() == 128);
  //}
}

TEMPLATE_TEST_CASE("TreeBitset (un)set random 1/2", "[set]", uint16_t, uint32_t, uint64_t)
{
  for(const size_t max_elements_exp : max_elements_exp_vals)
  {
    auto [tb, bitset] = prepare_random_data<TestType>(max_elements_exp, 2);

    for(size_t idx = 0; idx < size(bitset); ++idx)
      REQUIRE(tb.is_free(idx) == bitset[idx]);

    SECTION("obtain() obtains valid indices")
    {
      const size_t n_free = std::count(begin(bitset), end(bitset), true);
      for(size_t i = 0; i < n_free; ++i)
        REQUIRE(bitset[tb.obtain_id()]);

      REQUIRE(decltype(tb)::invalid_id == tb.obtain_id());
      REQUIRE(decltype(tb)::invalid_id == tb.obtain_id());
      REQUIRE(decltype(tb)::invalid_id == tb.obtain_id());
    }
  }
}

TEMPLATE_TEST_CASE("TreeBitset ordered obtain", "[obtain]", uint16_t, uint32_t, uint64_t)
{
  for(const size_t max_elements_exp : max_elements_exp_vals)
  {
    TreeBitset<TreeBitsetConfig<TestType>> tb{max_elements_exp};
    INFO("elements = " << tb.max_elements());
    for(size_t idx = 0; idx < tb.max_elements(); ++idx)
    {
      INFO("idx = " << idx);
      REQUIRE(tb.obtain_id() == idx);
      for(size_t i = 0; i < tb.max_elements(); ++i)
      {
        const bool should_be_free = i > idx;
        REQUIRE(tb.is_free(i) == should_be_free);
      }
    }

    SECTION("returns invalid id when no free elements left")
    {
      REQUIRE(tb.obtain_id() == decltype(tb)::invalid_id);
      REQUIRE(tb.obtain_id() == decltype(tb)::invalid_id);
      REQUIRE(tb.obtain_id() == decltype(tb)::invalid_id);
    }
  }
}

TEMPLATE_TEST_CASE("Invalid max_id by default", "[max_id]", uint16_t, uint32_t, uint64_t)
{
  TreeBitset<TreeBitsetConfig<TestType>> tb{2};
  REQUIRE(decltype(tb)::invalid_id == tb.max_used_id());
}

TEMPLATE_TEST_CASE("Determines max_id after unsetting random 1/2", "[max_id]", uint16_t, uint32_t, uint64_t)
{
  for(const size_t max_elements_exp : max_elements_exp_vals)
  {
    auto [tb, bitset] = prepare_random_data<TestType>(max_elements_exp, 2);
    for(auto it = crbegin(bitset); it != crend(bitset); ++it)
    {
      if(*it == false)
      {
        const size_t max_id = size(bitset) - std::distance(crbegin(bitset), it) - 1;
        REQUIRE(tb.max_used_id() == max_id);
        break;
      }
    }
  }
}

TEMPLATE_TEST_CASE(
  "Determines max_id after obtaining 1/2 elements at each step", "[max_id]", uint16_t, uint32_t, uint64_t)
{
  for(const size_t max_elements_exp : max_elements_exp_vals)
  {
    TreeBitset<TreeBitsetConfig<TestType>> tb{max_elements_exp};
    for(size_t idx = 0; idx < tb.max_elements() / 2; ++idx)
    {
      const size_t new_id = tb.obtain_id();
      REQUIRE(tb.max_used_id() == new_id);
    }
  }
}

TEMPLATE_TEST_CASE("Determines max_id after obtaining 1/2 elements and freeing max element at each step",
                   "[max_id]",
                   uint16_t,
                   uint32_t,
                   uint64_t)
{
  for(const size_t max_elements_exp : max_elements_exp_vals)
  {
    TreeBitset<TreeBitsetConfig<TestType>> tb{max_elements_exp};
    for(size_t idx = 0; idx < tb.max_elements() / 2; ++idx)
    {
      const size_t new_id = tb.obtain_id();
      REQUIRE(tb.max_used_id() == new_id);
      tb.set_free(new_id, true);
      if(idx == 0)
        REQUIRE(tb.max_used_id() == decltype(tb)::invalid_id);
      else
        REQUIRE(tb.max_used_id() == new_id - 1);
      // reobtain ID
      tb.obtain_id();
    }
  }
}

TEMPLATE_TEST_CASE("Determines max_id at each step while freeing IDs from max to min",
                   "[max_id]",

                   uint16_t,
                   uint32_t,
                   uint64_t)
{
  for(const size_t max_elements_exp : max_elements_exp_vals)
  {
    auto [tb, bitset] = prepare_random_data<TestType>(max_elements_exp, 2);

    for(auto it = crbegin(bitset); it != crend(bitset); ++it)
    {
      if(*it == false)
      {
        const size_t max_id = size(bitset) - std::distance(crbegin(bitset), it) - 1;
        REQUIRE(tb.max_used_id() == max_id);
        tb.set_free(max_id, true);
      }
    }
  }
}

TEMPLATE_TEST_CASE("Determines max_id at each step while freeing IDs in random order",
                   "[max_id]",
                   uint16_t,
                   uint32_t,
                   uint64_t)
{
  for(const size_t max_elements_exp : max_elements_exp_vals)
  {
    auto [tb, bitset] = prepare_random_data<TestType>(max_elements_exp, 2);

    for(size_t idx = 0; idx < tb.max_elements(); ++idx)
      REQUIRE(bitset[idx] == tb.is_free(idx));

    std::vector<size_t> max_id_history;
    for(auto it = begin(bitset); it != end(bitset); ++it)
    {
      if(*it == false)
        max_id_history.emplace_back(static_cast<size_t>(std::distance(begin(bitset), it)));
    }
    size_t * max_id = &max_id_history.back();

    std::vector<size_t> shuffled_ids{max_id_history};
    std::shuffle(begin(shuffled_ids), end(shuffled_ids), g);
    std::unordered_set<size_t> freed_ids;
    for(const size_t id : shuffled_ids)
    {
      freed_ids.emplace(id);
      INFO("id: " << id);
      REQUIRE(tb.max_used_id() == *max_id);
      tb.set_free(id, true);
      if(id == *max_id)
      {
        while(freed_ids.count(*max_id))
          --max_id;
      }
    }
  }
}

TEMPLATE_TEST_CASE("Used IDs iterator", "[iter]", uint16_t, uint32_t, uint64_t)
{
  for(const size_t max_elements_exp : max_elements_exp_vals)
  {
    TreeBitset<TreeBitsetConfig<TestType>> empty{max_elements_exp};
    for(size_t id : empty.used_ids_iter())
    {
      (void)id;
      REQUIRE(false);
    }

    auto [tb, bitset] = prepare_random_data<TestType>(max_elements_exp, 2);
    for(size_t id : tb.used_ids_iter())
    {
      INFO("id: " << id);
      INFO("max elements: " << tb.max_elements());
      REQUIRE(!bitset[id]);
      bitset[id] = true;
    }
    size_t id = 0;
    for(const bool bit : bitset)
    {
      INFO("id: " << id);
      REQUIRE(bit);
      id++;
    }
  }
}

TEMPLATE_TEST_CASE("(Un)packing", "[pack]", uint16_t, uint32_t, uint64_t)
{
  for(const size_t max_elements_exp : max_elements_exp_vals)
  {
    auto [tb, _] = prepare_random_data<TestType>(max_elements_exp, 2);

    std::vector<RLEBitAbbreviation> abbreviations;
    std::vector<TestType>           packed_blocks;
    tb.pack([&abbreviations](const RLEBitAbbreviation & a) { abbreviations.emplace_back(a); },
            [&packed_blocks](const TestType block) { packed_blocks.emplace_back(block); });

    REQUIRE(!packed_blocks.empty());

    auto unpacked =
      decltype(tb)::unpack(max_elements_exp, packed_blocks.data(), abbreviations.data(), size(abbreviations));

    REQUIRE(unpacked == tb);
  }
}

/// BENCHMARKS

TEST_CASE("TreeBitset<uint64> with 2^23 elements", "[bench]")
{
  BENCHMARK("init + obtain all in order")
  {
    TreeBitset tb{23};
    for(size_t idx = 0; idx < tb.max_elements(); ++idx)
      tb.obtain_id();
    REQUIRE_FALSE(tb.is_free(123));
  };

  BENCHMARK_ADVANCED("obtain 1024 in order")(Catch::Benchmark::Chronometer meter)
  {
    TreeBitset<> tb{23};
    meter.measure([&] {
      for(size_t idx = 0; idx < 1024; ++idx)
        tb.obtain_id();
      REQUIRE_FALSE(tb.is_free(123));
    });
  };

  BENCHMARK_ADVANCED("obtain all in order")(Catch::Benchmark::Chronometer meter)
  {
    TreeBitset<> tb{23};
    meter.measure([&] {
      for(size_t idx = 0; idx < tb.max_elements(); ++idx)
        tb.obtain_id();
      REQUIRE_FALSE(tb.is_free(123));
    });
  };

  BENCHMARK_ADVANCED("obtain half in order")(Catch::Benchmark::Chronometer meter)
  {
    TreeBitset<> tb{23};
    const size_t n_elements = tb.max_elements() / 2;
    meter.measure([&] {
      for(size_t idx = 0; idx < n_elements; ++idx)
        tb.obtain_id();
      REQUIRE_FALSE(tb.is_free(123));
    });
  };

  BENCHMARK_ADVANCED("obtain half - random free order")(Catch::Benchmark::Chronometer meter)
  {
    TreeBitset<> tb{23};

    const size_t max_elements = tb.max_elements();
    for(size_t idx = 0; idx < max_elements / 2; ++idx)
      tb.set_free(g() % max_elements, false);

    meter.measure([&] {
      const size_t max_elements = tb.max_elements();
      for(size_t idx = 0; idx < max_elements / 2; ++idx)
        tb.obtain_id();
      return tb.is_free(0);
    });
  };

  BENCHMARK_ADVANCED("set all unfree manually")(Catch::Benchmark::Chronometer meter)
  {
    TreeBitset<> tb{23};

    const size_t max_elements = tb.max_elements();
    for(size_t idx = 0; idx < max_elements / 2; ++idx)
      tb.set_free(g() % max_elements, false);

    meter.measure([&] {
      const size_t max_elements = tb.max_elements();
      for(size_t idx = 0; idx < max_elements; ++idx)
        tb.set_free(idx, false);
      REQUIRE_FALSE(tb.is_free(123));
    });
  };

  BENCHMARK_ADVANCED("pack+unpack to a pre-allocated buffers")(Catch::Benchmark::Chronometer meter)
  {
    TreeBitset<> tb{23};

    const size_t max_elements = tb.max_elements();
    for(size_t idx = 0; idx < max_elements / 2; ++idx)
      tb.set_free(g() % max_elements, false);

    std::vector<uint64_t> packed_blocks(tb.max_elements(), 0);
    size_t                packed_blocks_iter = 0;

    std::vector<RLEBitAbbreviation> abbreviations(tb.max_elements(), RLEBitAbbreviation{});
    size_t                          abbreviations_iter = 0;

    meter.measure([&] {
      // need to reset buffer positions, since it'll be run many times
      packed_blocks_iter = 0;
      abbreviations_iter = 0;
      tb.pack([&](const RLEBitAbbreviation & a) { abbreviations[abbreviations_iter++] = a; },
              [&](const uint64_t block) { packed_blocks[packed_blocks_iter++] = block; });

      auto unpacked =
        TreeBitset<>::unpack(23, packed_blocks.data(), abbreviations.data(), size(abbreviations));
      REQUIRE(unpacked.max_elements() == tb.max_elements());
    });
  };
}
