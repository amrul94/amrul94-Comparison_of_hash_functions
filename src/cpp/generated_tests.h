#ifndef THESIS_WORK_WORDS_TESTS_H
#define THESIS_WORK_WORDS_TESTS_H

#include <deque>
#include <execution>
#include <fstream>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include <boost/format.hpp>
#include <boost/json.hpp>

#include <pcg_random.hpp>

#include "concurrency.h"
#include "generators.h"
#include "test_parameters.h"
#include "log_duration.h"

constexpr int KILOBYTE = 1024;
constexpr int FOUR_KILOBYTES = KILOBYTE * 4;

namespace tests {
    namespace out {
        OutputJson GetGenTestJson(const GenBlocksParameters& gbp, ReportsRoot& reports_root);
    }

    template<typename HashStruct>
    auto HashTestWithGenBlocks(std::vector<pcg64>& generators, const HashStruct& hs, const GenBlocksParameters& gbp,
                               ReportsRoot& reports_root);

    template<typename HashStructs>
    void TestWithGeneratedBlocks(std::vector<pcg64>& generators, const HashStructs& hash_vec,
                                 const GenBlocksParameters& gbp, ReportsRoot& reports_root);


    void RunCollTestNormal(uint16_t words_length, size_t num_threads, ReportsRoot& reports_root);

    void RunCollTestWithMask(uint16_t words_length, size_t num_threads, ReportsRoot& reports_root);

    void RunTestWithGeneratedBlocks(uint16_t words_length, ReportsRoot& reports_root);


    // ===============================================================================

    template<typename Hasher>
    auto HashTestWithGenBlocks(const Hasher& hasher, const GenBlocksParameters& gbp, ReportsRoot& reports_root) {
        LOG_DURATION_STREAM("\t\ttime", reports_root.logger);
        reports_root.logger << boost::format("\n\t%1%: \n") % hasher.name;

        std::vector<pcg64> generators = GetGenerators(gbp.num_threads, (gbp.num_keys * gbp.words_length) / 8);   \
        // Выделить coll_flags, collisions, num_collisions и мьютекс (в будущем) в отдельный класс (например, Counters).
        // При этом подсчет коллизий вынести в метод класса AddHash
        const auto num_hashes = static_cast<uint64_t>(std::pow(2, gbp.test_bits));
        std::deque<bool> coll_flags(num_hashes, false);
        boost::json::object collisions;
        uint64_t num_collisions = 0;

        size_t num_words = (1 << (gbp.test_bits >> 1));
        int step = (gbp.test_bits != 24) ? 1 : 2;

        for (size_t i = 0; num_words <= gbp.num_keys; num_words <<= step) {
            for (; i < num_words; ++i) {
                std::string str = GenerateRandomDataBlock(generators.back(), gbp.words_length);
                const auto hash = static_cast<uint64_t>(hasher.hash(str));
                const uint64_t modified = ModifyHash(gbp, hash);
                bool& coll_flag = coll_flags[modified];
                const uint64_t tmp = coll_flag ? 1 : 0;
                num_collisions += tmp;
                coll_flag = true;
            }

            reports_root.logger << boost::format("\t\t%1% words:\t%2% collisions\n") % num_words % num_collisions;
            std::string index = std::to_string(num_words);
            collisions[index] = num_collisions;
        }

        for (const auto& [num_keys, num_cols] : collisions) {

        }

        auto hash_name = static_cast<boost::json::string>(hasher.name);
        if (gbp.mode == TestFlag::MASK) {
            hash_name += " (mask " + std::to_string(gbp.test_bits) + " bits)";
        }
        boost::json::object result;
        result[hash_name] = collisions;
        return std::pair{std::move(hash_name), std::move(collisions)};
    }

    // Возможно стоит поделить эту функции на части, так как она очень большая
    template<typename Hashes>
    void TestWithGeneratedBlocks(const Hashes& hashes, const GenBlocksParameters& gbp, ReportsRoot& reports_root) {
        reports_root.logger << "--- START " << gbp.hash_bits << " BITS TEST ---" << std::endl;

        auto out_json = out::GetGenTestJson(gbp, reports_root);

        boost::json::object collisions;
        std::mutex local_mutex;

        for (const auto& hasher : hashes) {
            auto [hash_name, counters] = HashTestWithGenBlocks(hasher, gbp, reports_root);
            collisions[hash_name] = std::move(counters);
        }

        out_json.obj["Collisions"] = collisions;
        out_json.out << out_json.obj;

        reports_root.logger << "\n--- END " << gbp.hash_bits << " BITS TEST ---" << std::endl << std::endl;
    }
}

#endif //THESIS_WORK_WORDS_TESTS_H