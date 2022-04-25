#include <random>

#include "cityhash/inline/city.h"
#include "farmhash/inline/farmhash.h"
#include "fasthash/inline/fasthash.h"
#include <metro_hash/metrohash64.h>
#include "murmur_hash/inline/MurmurHash.h"
#include <nmhash/nmhash.h>
#include <mx3/mx3.h>
#include "pearson/inline/pearson.h"
#include <pengyhash/pengyhash.h>
#include "spooky_hash/inline/spooky.h"
#include <super_fast_hash/super_fast_hash.h>
#include <t1ha/t1ha.h>
#include <wyhash/wyhash.h>
#include <wyhash/wyhash32.h>
#include <xx_hash/xxhash.h>
#include "siphash/inline/siphash.h"
#include <highwayhash/sip_hash.h>
#include <highwayhash/highwayhash.h>

#include <mum-mir/mir-hash.h>
#include <mum-mir/mum.h>

#include "hashes.h"
#include "speed_tests.h"
#include "generators.h"

namespace tests {
    namespace out {
        OutputJson GetSpeedTestJson(uint16_t hash_bits, size_t num_words, ReportsRoot& reports_root) {
            const std::filesystem::path report_test_dir = "Speed tests";
            const auto report_test_path = reports_root.root_path / report_test_dir;
            std::filesystem::create_directories(report_test_path);

            const std::filesystem::path report_name = std::to_string(hash_bits) + " bits.json";
            const std::filesystem::path out_path = report_test_path / report_name;
            std::ofstream out(out_path);

            boost::json::object obj;
            obj["Test name"] = "Speed Tests";
            obj["Bits"] = hash_bits;
            obj["Number of words"] = num_words;
            return OutputJson{std::move(obj), std::move(out)};
        }
    }

    struct StrView{};
    struct CharKeyIntLen{};
    struct CharKeyUintLen{};
    struct UcharKeyUintLen{};
    struct CharKeyIntLenSeed{};
    struct CharKeyUintLenSeed{};
    struct CharKeyUintLenSeeds{};
    struct UcharKeyUintLenSeed{};


    namespace args {
        constinit StrView str_view;
        constinit CharKeyIntLen char_key_int_len;
        constinit CharKeyUintLen char_key_uint_len;
        constinit UcharKeyUintLen uchar_key_uint_len;
        constinit CharKeyIntLenSeed char_key_int_len_seed;
        constinit CharKeyUintLenSeed char_key_uint_len_seed;
        constinit CharKeyUintLenSeeds char_key_uint_len_seeds;
        constinit UcharKeyUintLenSeed uchar_key_uint_len_seed;
    }

    namespace {
        template <typename Function>
        void HashTest(Function func, StrView, const std::string& hash_name, const std::vector<std::string>& words,
                      ReportsRoot& reports_root, boost::json::object& obj) {
            HashSpeed hs = HashSpeedTest(func, hash_name, words, reports_root);
            obj[hash_name] = hs.sec_time;
        }

        template <typename Function>
        void HashTest(Function func, CharKeyIntLen, const std::string& hash_name, const std::vector<std::string>& words,
                      ReportsRoot& reports_root, boost::json::object& obj) {
            auto lambda = [func] (std::string_view str) {
                const auto len = static_cast<int>(str.size());
                return func(str.data(), len);
            };
            HashTest(lambda, args::str_view, hash_name, words, reports_root, obj);
        }

        template <typename Function>
        void HashTest(Function func, CharKeyUintLen, const std::string& hash_name, const std::vector<std::string>& words,
                      ReportsRoot& reports_root, boost::json::object& obj) {
            auto lambda = [func] (std::string_view str) {
                return func(str.data(), str.size());
            };
            HashTest(lambda, args::str_view, hash_name, words, reports_root, obj);
        }

        template <typename Function>
        void HashTest(Function func, UcharKeyUintLen, const std::string& hash_name, const std::vector<std::string>& words,
                      ReportsRoot& reports_root, boost::json::object& obj) {
            auto lambda = [func] (std::string_view str) {
                const auto* key = reinterpret_cast<const uint8_t*>(str.data());
                return func(key, str.size());
            };
            HashTest(lambda, args::str_view, hash_name, words, reports_root, obj);
        }

        template <typename Function>
        void HashTest(Function func, CharKeyIntLenSeed, const std::string& hash_name, const std::vector<std::string>& words,
                      ReportsRoot& reports_root, boost::json::object& obj) {
            auto lambda = [func] (std::string_view str) {
                const auto len = static_cast<int>(str.size());
                return func(str.data(), len, SEED_64_1);
            };
            HashTest(lambda, args::str_view, hash_name, words, reports_root, obj);
        }

        template <typename Function>
        void HashTest(Function func, CharKeyUintLenSeed, const std::string& hash_name, const std::vector<std::string>& words,
                      ReportsRoot& reports_root, boost::json::object& obj) {
            auto lambda = [func] (std::string_view str) {
                return func(str.data(), str.size(), SEED_64_1);
            };
            HashTest(lambda, args::str_view, hash_name, words, reports_root, obj);
        }

        template <typename Function>
        void HashTest(Function func, CharKeyUintLenSeeds, const std::string& hash_name, const std::vector<std::string>& words,
                      ReportsRoot& reports_root, boost::json::object& obj) {
            auto lambda = [func] (std::string_view str) {
                return func(str.data(), str.size(), SEED_64_1, SEED_64_2);
            };
            HashTest(lambda, args::str_view, hash_name, words, reports_root, obj);
        }

        template <typename Function>
        void HashTest(Function func, UcharKeyUintLenSeed, const std::string& hash_name, const std::vector<std::string>& words,
                      ReportsRoot& reports_root, boost::json::object& obj) {
            auto lambda = [func] (std::string_view str) {
                const auto* key = reinterpret_cast<const uint8_t*>(str.data());
                return func(key, str.size(), 0);
            };
            HashTest(lambda, args::str_view, hash_name, words, reports_root, obj);
        }

        template <typename PearsonClass>
        void PearsonClassTest(PearsonClass pearson_hash, const std::vector<std::string>& words,
                      ReportsRoot& reports_root, boost::json::object& obj) {
            std::string hash_name = "PearsonHash";
            pearson_hash.Init();
            HashTest(pearson_hash, args::str_view, hash_name, words, reports_root, obj);
        }

        template <typename PearsonFunc>
        void PearsonFuncTest(PearsonFunc pearson_hash, const std::vector<std::string>& words,
                             ReportsRoot& reports_root, boost::json::object& obj) {
            std::string hash_name = "PearsonHash";
            pearson_inline::pearson_hash_init();
            HashTest(pearson_hash, args::uchar_key_uint_len_seed, hash_name, words, reports_root, obj);
        }


        template <typename BaseFunc>
        void FNV1aHashTest(BaseFunc func, uint16_t bits, const std::vector<std::string>& words, ReportsRoot& reports_root,
                                boost::json::object& obj) {
            std::string hash_name = "FNV-1a Hash";

            auto lambda = [func, bits](std::string_view str) {
                static const uint32_t mask = (((uint32_t)1 << bits)-1); /* i.e., (u_int32_t)0xffff */
                const auto len = static_cast<int>(str.size());
                uint32_t hash = func(str.data(), len, SEED_64_1);
                hash = (hash >> bits) ^ (hash & mask);
                return hash;
            };
            HashTest(lambda, args::str_view, hash_name, words, reports_root, obj);
        }

        template<typename UintT>
        void BuzHashTest(const std::vector<std::string>& words, ReportsRoot& reports_root, boost::json::object& obj) {
            std::string hash_name = "BuzHash";

            CyclicHash<UintT> hasher{4096, sizeof(UintT) * 8};
            auto lambda = [&hasher](std::string_view str) {
                return hasher.hash(str);
            };

            HashTest(lambda, args::str_view, hash_name, words, reports_root, obj);
        }

        template<typename UintT>
        void SpookyHashTest(const std::vector<std::string>& words, ReportsRoot& reports_root, boost::json::object& obj) {
            std::string hash_name = "SpookyHash";

            auto lambda = [](std::string_view str) {
                uint64_t hash1 = SEED_64_1, seed = SEED_64_2;
                spooky_inline::spooky_hash128(str.data(), str.size(), &hash1, &seed);
                return static_cast<UintT>(hash1);
            };
            HashTest(lambda, args::str_view, hash_name, words, reports_root, obj);
        }

        template<typename UintT>
        void FastHash1To31Test(const std::vector<std::string>& words, ReportsRoot& reports_root, boost::json::object& obj) {
            std::string hash_name = "FastHash";

            auto lambda = [](std::string_view str) {
                uint32_t h = fasthash_inline::fasthash32(str.data(), str.size(), SEED_32);
                return static_cast<UintT>(h - (h >> 16));
            };
            HashTest(lambda, args::str_view, hash_name, words, reports_root, obj);
        }

        template<typename UintT>
        void FastHash32To63Test(const std::vector<std::string>& words, ReportsRoot& reports_root, boost::json::object& obj) {
            std::string hash_name = "FastHash";

            auto lambda = [](std::string_view str) {
                uint64_t h = fasthash_inline::fasthash64(str.data(), str.size(), SEED_64_1);
                return static_cast<UintT>(h - (h >> 32));
            };

            HashTest(lambda, args::str_view, hash_name, words, reports_root, obj);
        }

        void FastHash64Test(const std::vector<std::string>& words, ReportsRoot& reports_root, boost::json::object& obj) {
            std::string hash_name = "FastHash";

            auto lambda = [](std::string_view str) {
                return fasthash_inline::fasthash64(str.data(), str.size(), SEED_64_1);
            };
            HashTest(lambda, args::str_view, hash_name, words, reports_root, obj);
        }

        void MetroHashTest(const std::vector<std::string>& words, ReportsRoot& reports_root, boost::json::object& obj) {
            std::string hash_name = "MetroHash64";

            auto lambda = [](std::string_view str) {
                uint64_t hash = 0;
                std::vector<uint8_t> hash_array(8, 0);
                MetroHash64::Hash(reinterpret_cast<const uint8_t*>(str.data()), str.size(), hash_array.data(), SEED_64_1);
                std::memcpy(&hash, hash_array.data(), 8);
                return hash;
            };
            HashTest(lambda, args::str_view, hash_name, words, reports_root, obj);
        }

        void WyHash64Test(const std::vector<std::string>& words, ReportsRoot& reports_root, boost::json::object& obj) {
            std::string hash_name = "wyhash64";

            auto lambda = [](std::string_view str) {
                return wyhash(str.data(), str.size(), SEED_64_1, _wyp);
            };
            HashTest(lambda, args::str_view, hash_name, words, reports_root, obj);
        }

        template <typename BaseFunc>
        void SipHashTest(BaseFunc func, const std::string& hash_name, const std::vector<std::string>& words,
                         ReportsRoot& reports_root, boost::json::object& obj) {
            unsigned char key[16] = {SEED_8_1, SEED_8_2, SEED_8_3, SEED_8_4,
                                     SEED_8_5, SEED_8_6, SEED_8_7, SEED_8_8,
                                     SEED_8_9, SEED_8_10, SEED_8_11, SEED_8_12,
                                     SEED_8_13, SEED_8_14, SEED_8_15, SEED_8_16};

            auto lambda = [func, &key](std::string_view str) {
                return func(key, (const unsigned char *)str.data(), str.size());
            };
            HashTest(lambda, args::str_view, hash_name, words, reports_root, obj);
        }

        template <typename BaseFunc>
        void GoogleSipHashTest(BaseFunc func, const std::string& hash_name, const std::vector<std::string>& words,
                         ReportsRoot& reports_root, boost::json::object& obj) {
            const highwayhash::HH_U64 key[2] = {SEED_64_1, SEED_64_2};

            auto lambda = [func, &key](std::string_view str) {
                return func(key, str.data(), str.size());
            };
            HashTest(lambda, args::str_view, hash_name, words, reports_root, obj);
        }

        void HighwayHashTest(const std::vector<std::string>& words, ReportsRoot& reports_root, boost::json::object& obj) {
            std::string hash_name = "HighwayHash";

            auto lambda = [](std::string_view str) {
                using namespace highwayhash;

                static const HHKey key HH_ALIGNAS(32) = {SEED_64_1, SEED_64_2, SEED_64_3, SEED_64_4};
                HHStateT<HH_TARGET> state(key);
                HHResult64 result;
                HighwayHashT(&state, str.data(), str.size(), &result);

                return result;
            };
            HashTest(lambda, args::str_view, hash_name, words, reports_root, obj);
        }

        template<typename UintT>
        void SpeedTestT(const std::vector<std::string>& words, ReportsRoot& reports_root, boost::json::object& obj) {
            using namespace std::literals;

            HashTest(DJB2Hash<UintT>, args::char_key_uint_len, "DJB2 Hash"s, words, reports_root, obj);
            HashTest(SDBMHash<UintT>, args::char_key_uint_len, "SDBM Hash"s, words, reports_root, obj);
            HashTest(PJWHash<UintT>, args::char_key_uint_len, "PJW Hash"s, words, reports_root, obj);
            HashTest(one_at_a_time_hash<UintT>, args::uchar_key_uint_len, "One at a time"s, words, reports_root, obj);
            SpookyHashTest<UintT>(words, reports_root, obj);
        }
    }

    [[maybe_unused]] boost::json::object SpeedTests16(const std::vector<std::string>& words, ReportsRoot& reports_root) {
        boost::json::object obj;

        FNV1aHashTest(FNV32a, 16, words, reports_root, obj);
        SpeedTestT<uint16_t>(words, reports_root, obj);
        FastHash1To31Test<uint16_t>(words, reports_root, obj);
        PearsonClassTest(hfl::PearsonHash16{}, words, reports_root, obj);
        BuzHashTest<uint16_t>(words, reports_root, obj);

        return obj;
    }

    [[maybe_unused]] boost::json::object SpeedTests24(const std::vector<std::string>& words, ReportsRoot& reports_root) {
        boost::json::object obj;

        FNV1aHashTest(FNV32a, 24, words, reports_root, obj);
        SpeedTestT<hfl::uint24_t>(words, reports_root, obj);
        FastHash1To31Test<hfl::uint24_t>(words, reports_root, obj);
        PearsonClassTest(hfl::PearsonHash24{}, words, reports_root, obj);

        return obj;
    }

    [[maybe_unused]] boost::json::object SpeedTests32(const std::vector<std::string>& words, ReportsRoot& reports_root) {
        boost::json::object obj;

        using namespace std::literals;


        HashTest(FNV32a, args::char_key_int_len_seed, "FNV-1a Hash"s, words, reports_root, obj);
        SpeedTestT<uint32_t>(words, reports_root, obj);
        FastHash32To63Test<uint32_t>(words, reports_root, obj);
        PearsonFuncTest(pearson_inline::pearson_hash_32, words, reports_root, obj);
        BuzHashTest<uint32_t>(words, reports_root, obj);

        HashTest(SuperFastHash, args::char_key_int_len, "SuperFastHash"s, words, reports_root, obj);
        HashTest(murmur_inline::MurmurHash1, args::char_key_int_len_seed, "MurmurHash1"s, words, reports_root, obj);
        HashTest(murmur_inline::MurmurHash2, args::char_key_int_len_seed, "MurmurHash2"s, words, reports_root, obj);
        HashTest(murmur_inline::MurmurHash2A, args::char_key_int_len_seed, "MurmurHash2A"s, words, reports_root, obj);
        HashTest(murmur_inline::MurmurHash3_x86_32, args::char_key_int_len_seed, "MurmurHash3"s, words, reports_root, obj);
        HashTest(city::s_inline::CityHash32, args::char_key_uint_len, "CityHash32"s, words, reports_root, obj);
        HashTest(util::s_inline::Hash32, args::char_key_uint_len, "FarmHash32"s, words, reports_root, obj);
        HashTest(util::s_inline::Hash32WithSeed, args::char_key_uint_len_seed, "FarmHash32 with seed"s, words, reports_root, obj);

        // no inline
        HashTest(t1ha0_32le, args::char_key_uint_len_seed, "T1HA0 32le hash"s, words, reports_root, obj);
        HashTest(t1ha0_32be, args::char_key_uint_len_seed, "T1HA0 32be hash"s, words, reports_root, obj);

        HashTest(XXH32, args::char_key_uint_len_seed, "xxHash32"s, words, reports_root, obj);
        HashTest(wyhash32, args::char_key_uint_len_seed, "wyHash32"s, words, reports_root, obj);
        HashTest(NMHASH32, args::char_key_uint_len_seed, "NMHASH32"s, words, reports_root, obj);
        HashTest(NMHASH32X, args::char_key_uint_len_seed, "NMHASH32X"s, words, reports_root, obj);
        SipHashTest(halfsiphash, "HalfSipHash", words, reports_root, obj);

        return obj;
    }

    [[maybe_unused]] boost::json::object SpeedTests48(const std::vector<std::string>& words, ReportsRoot& reports_root) {
        boost::json::object obj;

        FNV1aHashTest(FNV64a, 48, words, reports_root, obj);
        SpeedTestT<hfl::uint48_t>(words, reports_root, obj);
        FastHash32To63Test<hfl::uint48_t>(words, reports_root, obj);

        return obj;
    }

    [[maybe_unused]] boost::json::object SpeedTests64(const std::vector<std::string>& words, ReportsRoot& reports_root) {
        boost::json::object obj;

        using namespace std::literals;

        HashTest(FNV64a, args::char_key_int_len_seed, "FNV-1a Hash", words, reports_root, obj);
        SpeedTestT<uint64_t>(words, reports_root, obj);
        FastHash64Test(words, reports_root, obj);
        PearsonFuncTest(pearson_inline::pearson_hash_64, words, reports_root, obj);
        BuzHashTest<uint64_t>(words, reports_root, obj);

        HashTest(murmur_inline::MurmurHash64A, args::char_key_int_len_seed, "MurmurHash64A"s, words, reports_root, obj);
        HashTest(city::s_inline::CityHash64, args::char_key_uint_len, "CityHash64"s, words, reports_root, obj);
        HashTest(city::s_inline::CityHash64WithSeed, args::char_key_uint_len_seed, "CityHash64 with seed"s, words, reports_root, obj);
        HashTest(city::s_inline::CityHash64WithSeeds, args::char_key_uint_len_seeds, "CityHash64 with seeds"s, words, reports_root, obj);
        HashTest(util::s_inline::Hash64, args::char_key_uint_len, "FarmHash64"s, words, reports_root, obj);
        HashTest(util::s_inline::Hash64WithSeed, args::char_key_uint_len_seed, "FarmHash64 with seed"s, words, reports_root, obj);
        HashTest(util::s_inline::Hash64WithSeeds, args::char_key_uint_len_seeds, "FarmHash64 with seeds"s, words, reports_root, obj);
        MetroHashTest(words, reports_root, obj);

        // no inline
        HashTest(t1ha0_ia32aes_avx2, args::char_key_uint_len_seed, "T1HA0 AVX2 hash"s, words, reports_root, obj);
        HashTest(t1ha1_le, args::char_key_uint_len_seed, "T1HA1 le hash"s, words, reports_root, obj);
        HashTest(t1ha1_be, args::char_key_uint_len_seed, "T1HA1 be hash"s, words, reports_root, obj);
        HashTest(t1ha2_atonce, args::char_key_uint_len_seed, "T1HA2 atonce hash"s, words, reports_root, obj);

        HashTest(XXH64, args::char_key_uint_len_seed, "xxHash64"s, words, reports_root, obj);
        HashTest(XXH3_64bits, args::char_key_uint_len, "XXH3 64 bits"s, words, reports_root, obj);
        HashTest(XXH3_64bits_withSeed, args::char_key_uint_len_seed, "XXH3 64 bits with seed"s, words, reports_root, obj);

        WyHash64Test(words, reports_root, obj);
        HashTest(pengyhash, args::char_key_uint_len_seed, "PengyHash"s, words, reports_root, obj);
        HashTest(mx3::hash, args::uchar_key_uint_len_seed, "MX3 Hash"s, words, reports_root, obj);
        SipHashTest(siphash, "SipHash", words, reports_root, obj);
        SipHashTest(siphash13, "SipHash13", words, reports_root, obj);
        GoogleSipHashTest(highwayhash::SipHash, "SipHash  (Google Impl)", words, reports_root, obj);
        GoogleSipHashTest(highwayhash::SipHash13, "SipHash13  (Google Impl)", words, reports_root, obj);
        HighwayHashTest(words, reports_root, obj);
        HashTest(mum_hash, args::char_key_uint_len_seed, "MUM hash"s, words, reports_root, obj);
        HashTest(mir_hash, args::char_key_uint_len_seed, "mir hash"s, words, reports_root, obj);

        return obj;
    }

    #define RUN_SPEED_TESTS_IMPL(BITS, WORDS, ROOT)                                 \
        (ROOT).logger << boost::format("--- START %1% BITS TEST ---") % (BITS);                  \
        auto out_json##BITS = out::GetSpeedTestJson(BITS, (WORDS).size(), ROOT); \
        boost::json::object speed##BITS = SpeedTests##BITS(WORDS, ROOT);            \
        out_json##BITS.obj["Speed"] = speed##BITS;                                  \
        out_json##BITS.out << out_json##BITS.obj;                                   \
        (ROOT).logger << boost::format("--- END %1% BITS TEST ---\n\n") % (BITS)


    void RunSpeedTests(uint64_t num_blocks, uint32_t block_length, ReportsRoot& reports_root) {
        pcg64 rng;
        const auto random_blocks = GenerateRandomDataBlocks(rng, num_blocks, block_length);

        RUN_SPEED_TESTS_IMPL(16, random_blocks, reports_root);
        RUN_SPEED_TESTS_IMPL(24, random_blocks, reports_root);
        RUN_SPEED_TESTS_IMPL(32, random_blocks, reports_root);
        RUN_SPEED_TESTS_IMPL(48, random_blocks, reports_root);
        RUN_SPEED_TESTS_IMPL(64, random_blocks, reports_root);
    }
}




