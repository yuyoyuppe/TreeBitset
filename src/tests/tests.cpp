#include <tree_bitset/tree_bitset.hpp>

#include <array>
#include <vector>
#include <algorithm>

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

TEST_CASE("Correct tree configuration", "[properties]")
{
  SECTION("TreeBitset<uint64_t> with 1 element")
  {
    TreeBitset<uint64_t> tb{0};
    REQUIRE(tb.num_metadata_levels() == 0);
    REQUIRE(tb.num_element_blocks() == 1);
    REQUIRE(tb.num_metadata_blocks() == 0);
    REQUIRE(tb.max_elements() == 1);
  }
  SECTION("TreeBitset<uint64_t> with 64 elements")
  {
    TreeBitset<uint64_t> tb{6};
    REQUIRE(tb.num_metadata_levels() == 0);
    REQUIRE(tb.num_element_blocks() == 1);
    REQUIRE(tb.num_metadata_blocks() == 0);
    REQUIRE(tb.max_elements() == 64);
  }
  SECTION("TreeBitset<uint64_t> with 2^12 elements")
  {
    TreeBitset<uint64_t> tb{12};
    REQUIRE(tb.num_metadata_levels() == 1);
    REQUIRE(tb.num_element_blocks() == 64);
    REQUIRE(tb.num_metadata_blocks() == 1);
    REQUIRE(tb.max_elements() == 64 * 64);
  }

  SECTION("TreeBitset<uint64_t> with 2^13 elements")
  {
    TreeBitset<uint64_t> tb{13};
    REQUIRE(tb.num_metadata_levels() == 2);
    REQUIRE(tb.num_element_blocks() == 128);
    REQUIRE(tb.num_metadata_blocks() == 1 + 64);
    REQUIRE(tb.max_elements() == 64 * 64 * 2);
  }

  SECTION("TreeBitset<uint8_t> with 2^7 elements")
  {
    TreeBitset<uint8_t> tb{7};
    REQUIRE(tb.num_metadata_levels() == 2);
    REQUIRE(tb.num_element_blocks() == 16);
    REQUIRE(tb.num_metadata_blocks() == 1 + 8);
    REQUIRE(tb.max_elements() == 128);
  }
}

TEMPLATE_TEST_CASE("TreeBitset (un)set random 1000 elements", "[set]", uint8_t, uint16_t, uint32_t, uint64_t)
{
  const std::array max_elements_exp_vals = {5, 8, 10, 12};
  for(const size_t max_elements_exp : max_elements_exp_vals)
  {
    INFO("elements = " << (1 << max_elements_exp));
    TreeBitset<TestType> tb{max_elements_exp};
    std::vector<bool>    bitset(tb.max_elements(), true);
    for(int i = 0; i < 1000; ++i)
    {
      const size_t idx = rand() % size(bitset);
      if(rand() % 2)
      {
        bitset[idx] = false;
        tb.set_free(idx, false);
      }
      else
      {
        bitset[idx] = true;
        tb.set_free(idx, true);
      }
    }
    for(size_t idx = 0; idx < size(bitset); ++idx)
      REQUIRE(tb.is_free(idx) == bitset[idx]);

    SECTION("obtain() gets correct indices")
    {
      const size_t n_free = std::count(begin(bitset), end(bitset), true);
      for(size_t i = 0; i < n_free; ++i)
        REQUIRE(bitset[tb.obtain_id()]);

      REQUIRE(TreeBitset<TestType>::invalid_id == tb.obtain_id());
      REQUIRE(TreeBitset<TestType>::invalid_id == tb.obtain_id());
      REQUIRE(TreeBitset<TestType>::invalid_id == tb.obtain_id());
    }
  }
}

TEMPLATE_TEST_CASE("TreeBitset ordered obtain", "[obtain]", uint8_t, uint16_t, uint32_t, uint64_t)
{
  const std::array max_elements_exp_vals = {6, 7, 12, 13};
  for(const size_t max_elements_exp : max_elements_exp_vals)
  {
    INFO("elements = " << (1 << max_elements_exp));
    TreeBitset<TestType> tb{max_elements_exp};
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
      REQUIRE(tb.obtain_id() == TreeBitset<TestType>::invalid_id);
      REQUIRE(tb.obtain_id() == TreeBitset<TestType>::invalid_id);
      REQUIRE(tb.obtain_id() == TreeBitset<TestType>::invalid_id);
    }
  }
}
