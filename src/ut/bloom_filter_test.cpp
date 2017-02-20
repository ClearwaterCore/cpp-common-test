/**
 * @file bloom_filter_test.cpp
 *
 * Project Clearwater - IMS in the Cloud
 * Copyright (C) 2017  Metaswitch Networks Ltd
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version, along with the "Special Exception" for use of
 * the program along with SSL, set forth below. This program is distributed
 * in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details. You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * The author can be reached by email at clearwater@metaswitch.com or by
 * post at Metaswitch Networks Ltd, 100 Church St, Enfield EN2 6BQ, UK
 *
 * Special Exception
 * Metaswitch Networks Ltd  grants you permission to copy, modify,
 * propagate, and distribute a work formed by combining OpenSSL with The
 * Software, or a work derivative of such a combination, even if such
 * copying, modification, propagation, or distribution would otherwise
 * violate the terms of the GPL. You must comply with the GPL in all
 * respects for all of the code used other than OpenSSL.
 * "OpenSSL" means OpenSSL toolkit software distributed by the OpenSSL
 * Project and licensed under the OpenSSL Licenses, or a work based on such
 * software and licensed under the OpenSSL Licenses.
 * "OpenSSL Licenses" means the OpenSSL License and Original SSLeay License
 * under which the OpenSSL Project distributes the OpenSSL toolkit software,
 * as those licenses appear in the file LICENSE-OPENSSL.
 */

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "bloom_filter.h"

using ::testing::Return;
using ::testing::_;

//
// Bloom filter tests.
//
// Bloom filters are probabilistic data structures that can return incorrect
// results (false positives), which means these tests can spuriously fail. There
// isn't a good way around this, but the failure rate should be ~0.01%, which is
// sufficiently low as not to be a problem.
//

TEST(BloomFilterTest, NewBloomFilterIsEmpty)
{
  BloomFilter bf(10000, 1);
  EXPECT_FALSE(bf.check("Kermit"));
  EXPECT_FALSE(bf.check("MissPiggy"));
  EXPECT_FALSE(bf.check("Gonzo"));
  EXPECT_FALSE(bf.check("Animal"));
}

TEST(BloomFilterTest, OneBitPerItem)
{
  BloomFilter bf(10000, 1);

  bf.add("Kermit");
  bf.add("MissPiggy");

  EXPECT_TRUE(bf.check("Kermit"));
  EXPECT_TRUE(bf.check("MissPiggy"));
  EXPECT_FALSE(bf.check("Gonzo"));
  EXPECT_FALSE(bf.check("Animal"));
}

TEST(BloomFilterTest, TwoBitsPerItem)
{
  BloomFilter bf(10000, 2);

  bf.add("Kermit");
  bf.add("MissPiggy");

  EXPECT_TRUE(bf.check("Kermit"));
  EXPECT_TRUE(bf.check("MissPiggy"));
  EXPECT_FALSE(bf.check("Gonzo"));
  EXPECT_FALSE(bf.check("Animal"));
}

TEST(BloomFilterTest, ManyBitsPerItem)
{
  BloomFilter bf(100000, 10);

  bf.add("Kermit");
  bf.add("MissPiggy");

  EXPECT_TRUE(bf.check("Kermit"));
  EXPECT_TRUE(bf.check("MissPiggy"));
  EXPECT_FALSE(bf.check("Gonzo"));
  EXPECT_FALSE(bf.check("Animal"));
}

TEST(BloomFilterTest, NumEntriesAndFalsePositiveRate)
{
  BloomFilter* bf = BloomFilter::for_num_entries_and_fp_prob(2, 0.0001);

  bf->add("Kermit");
  bf->add("MissPiggy");

  EXPECT_TRUE(bf->check("Kermit"));
  EXPECT_TRUE(bf->check("MissPiggy"));
  EXPECT_FALSE(bf->check("Gonzo"));
  EXPECT_FALSE(bf->check("Animal"));

  delete bf; bf = nullptr;
}

TEST(BloomFilterTest, BadConstructorArguments)
{
  EXPECT_EQ(BloomFilter::for_num_entries_and_fp_prob(0, 0.5), nullptr);
  EXPECT_EQ(BloomFilter::for_num_entries_and_fp_prob(1, -0.5), nullptr);
  EXPECT_EQ(BloomFilter::for_num_entries_and_fp_prob(1, 1.5), nullptr);
}

TEST(BloomFilterTest, JsonSerializeDeserialize)
{
  BloomFilter bf(100000, 10);

  bf.add("Kermit");
  bf.add("MissPiggy");

  // Serialize and deserialize the bloom filter.
  BloomFilter* bf2 = BloomFilter::from_json(bf.to_json());

  EXPECT_NE(bf2, nullptr);
  EXPECT_TRUE(bf2->check("Kermit"));
  EXPECT_TRUE(bf2->check("MissPiggy"));
  EXPECT_FALSE(bf2->check("Gonzo"));
  EXPECT_FALSE(bf2->check("Animal"));
}

TEST(BloomFilterTest, JsonSerializeDeserializeEmpty)
{
  BloomFilter bf(100000, 10);

  // Serialize and deserialize the bloom filter.
  BloomFilter* bf2 = BloomFilter::from_json(bf.to_json());

  EXPECT_NE(bf2, nullptr);
  EXPECT_FALSE(bf2->check("Kermit"));
  EXPECT_FALSE(bf2->check("MissPiggy"));
  EXPECT_FALSE(bf2->check("Gonzo"));
  EXPECT_FALSE(bf2->check("Animal"));
}

TEST(BloomFilterTest, JsonDeserialize3rdParty)
{
  // A bloom filter that conforms to the JSON API but was generated by an
  // independent 3rd party implementation. It contains the strings "Kermit" and
  // "MissPiggy".
  const std::string json =
    "{\"bitmap\":\"J+i5Mg==\",\"total_bits\":32,\"bits_per_entry\":12,"
     "\"hash0\":{\"k0\":6547054200929830170,\"k1\":9813628641652032020},"
     "\"hash1\":{\"k0\":15888472079188754020,\"k1\":14822504794822470401}}";

  BloomFilter* bf = BloomFilter::from_json(json);

  EXPECT_NE(bf, nullptr);
  EXPECT_TRUE(bf->check("Kermit"));
  EXPECT_TRUE(bf->check("MissPiggy"));
  EXPECT_FALSE(bf->check("Gonzo"));
  EXPECT_FALSE(bf->check("Animal"));
}

TEST(BloomFilterTest, JsonDeserializeExtraFields)
{
  // A bloom filter that contains some additional fields that our implementation
  // does not recognize. This should be accepted, as it represents a
  // backwards-compatible change to the filter.
  const std::string json =
    "{\"bitmap\":\"J+i5Mg==\",\"total_bits\":32,\"bits_per_entry\":12,"
     "\"hash0\":{\"k0\":6547054200929830170,\"k1\":9813628641652032020},"
     "\"hash1\":{\"k0\":15888472079188754020,\"k1\":14822504794822470401},"
     "\"future\": \"unknown\"}";

  BloomFilter* bf = BloomFilter::from_json(json);

  EXPECT_NE(bf, nullptr);
  EXPECT_TRUE(bf->check("Kermit"));
  EXPECT_TRUE(bf->check("MissPiggy"));
  EXPECT_FALSE(bf->check("Gonzo"));
  EXPECT_FALSE(bf->check("Animal"));
}

TEST(BloomFilterTest, BadJson)
{
  // A string that is not valid JSON.
  EXPECT_EQ(BloomFilter::from_json("It's the muppet show"), nullptr);

  // A string that is valid JSON but not a valid bloom filter.
  EXPECT_EQ(BloomFilter::from_json("{}"), nullptr);

  // A string that is not a valid bloom filter because the bitmap is not valid
  // base64.
  const std::string json =
    "{\"bitmap\":\"^^^^^^^\",\"total_bits\":32,\"bits_per_entry\":12,"
     "\"hash0\":{\"k0\":6547054200929830170,\"k1\":9813628641652032020},"
     "\"hash1\":{\"k0\":15888472079188754020,\"k1\":14822504794822470401}}";
  EXPECT_EQ(BloomFilter::from_json(json), nullptr);
}
