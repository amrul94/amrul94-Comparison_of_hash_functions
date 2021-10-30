#include "helper_structs.h"

#include <cmath>
#include <limits>

std::string ModeToString(ModeFlag mode) {
    switch (mode) {
        case ModeFlag::NORMAL:
            return "Normal";
        case ModeFlag::BINS:
            return "Bins";
        case ModeFlag::MASK:
            return "Mask";
    }
}

uint64_t MaskShift(uint64_t src, uint16_t mask_bits, uint16_t shift) {
    const auto mask = static_cast<uint64_t>(std::pow(2, mask_bits)) - 1;
    return (src >> shift) & mask;
}

TestParameters::TestParameters(uint16_t hash_bits, uint16_t test_bits, ModeFlag mode)
        : TestParameters(hash_bits, test_bits, 0, mode) {
}

TestParameters::TestParameters(uint16_t hash_bits, uint16_t test_bits, uint64_t key_counts, ModeFlag mode)
        : hash_bits(hash_bits)
        , test_bits(test_bits)
        , key_count(key_counts)
        , mode(mode) {
}

uint64_t TestParameters::GiveDivisor(uint16_t degree) {
    return static_cast<uint64_t>(std::pow(2, degree));
}

CheckParameters::CheckParameters(uint16_t hash_bits, uint16_t test_bits, ModeFlag mode)
        : TestParameters(hash_bits, test_bits, mode) {
    SetParameters();
}

void CheckParameters::SetParameters() {
    switch (mode) {
        case ModeFlag::NORMAL:
            SetNormalMode();
            break;
        case ModeFlag::BINS:
            SetBinsMode();
            break;
        case ModeFlag::MASK:
            SetMaskMode();
            break;
        default:
            break;
    }
}

void CheckParameters::SetNormalMode() {
    hfl::uint24_t max_uint = 0;
    switch (test_bits) {
        case 16:
            max_uint = std::numeric_limits<uint16_t>::max();
            break;
        case 24:
            max_uint = std::numeric_limits<hfl::uint24_t>::max();
            break;
        default:
            assert(false);
    }
    key_count = static_cast<uint64_t>(max_uint) + 1;
    buckets_count = key_count;
}

void CheckParameters::SetBinsMode() {
    key_count = std::numeric_limits<uint32_t>::max() + static_cast<uint64_t>(1);
    buckets_count = MAX_BINS_COUNT;
    switch (test_bits) {
        case 32:
            divisor = GiveDivisor(DIVIDER_FOR_32);
            break;
        case 64:
            divisor = GiveDivisor(DIVIDER_FOR_64);
            break;
        default:
            assert(false);
    }
}

void CheckParameters::SetMaskMode() {
    SetNormalMode();
}

WordsParameters::WordsParameters(uint16_t hash_bits, uint16_t test_bits, uint64_t word_counts, uint32_t length, ModeFlag mode)
        : TestParameters(hash_bits, test_bits, word_counts, mode)
        , words_length(length){
}

uint64_t ModifyHash(const TestParameters& tp, uint64_t hash) {
    switch (tp.mode) {
        case ModeFlag::NORMAL:
            return hash;
        case ModeFlag::MASK:
            return MaskShift(hash, tp.test_bits);
        case ModeFlag::BINS:
            const auto& cp = dynamic_cast<const CheckParameters&>(tp);
            return hash / cp.divisor;
    }
}

