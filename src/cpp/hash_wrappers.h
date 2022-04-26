#ifndef THESIS_WORK_HASH_WRAPPERS_H
#define THESIS_WORK_HASH_WRAPPERS_H

#include <array>
#include <concepts>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>

#include <boost/multiprecision/cpp_int.hpp>

#include <hash_functions.h>
#include <rolling_hash/cyclichash.h>

// HFL = Hash function library
namespace hfl {
    using uint12_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<8, 8,
            boost::multiprecision::unsigned_magnitude,
            boost::multiprecision::unchecked, void>>;

    using uint24_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<24, 24,
            boost::multiprecision::unsigned_magnitude,
            boost::multiprecision::unchecked, void>>;

    using uint48_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<48, 48,
            boost::multiprecision::unsigned_magnitude,
            boost::multiprecision::unchecked, void>>;

    template<typename T>
    concept UnsignedIntegral = (std::is_integral_v<T> && !std::is_signed_v<T>) || std::is_same_v<T, uint24_t>
            || std::is_same_v<T, uint48_t>;
}

namespace hfl::wrappers::detail {
    template<UnsignedIntegral UintT>
    class BaseHashWrapper {
    public:
        UintT Hash(const std::string& str) const;
        UintT Hash(std::ifstream& file) const;
        UintT Hash(std::integral auto number) const;
        virtual ~BaseHashWrapper() = default;

    private:
        [[nodiscard]] virtual UintT HashImpl(const char *message, size_t length) const = 0;
        static std::string ReadFile(std::ifstream& file);
    };


    template<UnsignedIntegral UintT>
    UintT BaseHashWrapper<UintT>::Hash(const std::string& str) const {
        return HashImpl(str.data(), str.size());
    }

    template<UnsignedIntegral UintT>
    UintT BaseHashWrapper<UintT>::Hash(std::ifstream& file) const {
        std::string binary_file = ReadFile(file);
        assert(!binary_file.empty());
        return Hash(binary_file);
    }

    template<UnsignedIntegral UintT>
    UintT BaseHashWrapper<UintT>::Hash(std::integral auto number) const {
        const char* bytes = reinterpret_cast<const char*>(reinterpret_cast<const void*>(&number));
        static const size_t length = sizeof(number);
        return HashImpl(bytes, length);
    }

    template<UnsignedIntegral UintT>
    std::string BaseHashWrapper<UintT>::ReadFile(std::ifstream& file) {
        assert(file);
        std::string result;
        size_t source_size = 0;
        do {
            char buff[1024];
            file.read(buff, sizeof buff);
            size_t read_size = file.gcount();
            source_size += read_size;
            result.append(buff, read_size);
        } while (file);
        return result;
    }
}

namespace hfl::wrappers {
    using BaseHash16Wrapper = detail::BaseHashWrapper<uint16_t>;
    using BaseHash24Wrapper = detail::BaseHashWrapper<uint24_t>;
    using BaseHash32Wrapper = detail::BaseHashWrapper<uint32_t>;
    using BaseHash48Wrapper = detail::BaseHashWrapper<uint48_t>;
    using BaseHash64Wrapper = detail::BaseHashWrapper<uint64_t>;

//----- Bernstein's hash DJB2 ------

    template<UnsignedIntegral UintT>
    class [[maybe_unused]] DJB2HashWrapper final : public detail::BaseHashWrapper<UintT> {
    private:
        [[nodiscard]] UintT HashImpl(const char *message, size_t length) const override;
    };

    template<UnsignedIntegral UintT>
    [[nodiscard]] UintT DJB2HashWrapper<UintT>::HashImpl(const char *message, size_t length) const {
        return DJB2Hash<UintT>(message, length);
    }


//----- Rolling Hash (BuzHash) -----

    template<UnsignedIntegral UintT>
    class [[maybe_unused]] BuzHashWrapper final : public detail::BaseHashWrapper<UintT> {
    public:
        BuzHashWrapper() noexcept = default;

    private:
        UintT HashImpl(const char *message, size_t length) const override;

        mutable CyclicHash<UintT, char> hasher_{4096, sizeof(UintT) * 8};
    };

    template<UnsignedIntegral UintT>
    UintT BuzHashWrapper<UintT>::HashImpl(const char *message, size_t length) const {
        std::string_view str(message, length);
        return hasher_.hash(str);
    }

//----------- CityHashes ----------

    class [[maybe_unused]] CityHash32Wrapper final : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] CityHash64Wrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] CityHash64WithSeedWrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] CityHash64WithSeedsWrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

//----------- FarmHashes ----------

    class [[maybe_unused]] FarmHash32Wrapper final : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] FarmHash32WithSeedWrapper final : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] FarmHash64Wrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] FarmHash64WithSeedWrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] FarmHash64WithSeedsWrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

//------------ FastHash ------------

    class [[maybe_unused]] FastHash16Wrapper final : public BaseHash16Wrapper {
    private:
        [[nodiscard]] uint16_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] FastHash24Wrapper final : public BaseHash24Wrapper {
    private:
        [[nodiscard]] uint24_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] FastHash32Wrapper final : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] FastHash48Wrapper final : public BaseHash48Wrapper {
    private:
        [[nodiscard]] uint48_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] FastHash64Wrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

//---------- FNV-1a hash -----------

    class [[maybe_unused]] FNV1aHash16Wrapper final : public BaseHash16Wrapper {
    private:
        [[nodiscard]] uint16_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] FNV1aHash24Wrapper final : public BaseHash24Wrapper {
    private:
        [[nodiscard]] uint24_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] FNV1aHash32Wrapper final : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] FNV1aHash48Wrapper final : public BaseHash48Wrapper {
    private:
        [[nodiscard]] uint48_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] FNV1aHash64Wrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

//---------- HighwayHash -----------

    class [[maybe_unused]] HighwayHashWrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

//--------- Jenkins hash -----------

    template<UnsignedIntegral UintT>
    class [[maybe_unused]] OneTimeHashWrapper final : public detail::BaseHashWrapper<UintT> {
    private:
        [[nodiscard]] UintT HashImpl(const char *message, size_t length) const override;
    };

    template<UnsignedIntegral UintT>
    [[nodiscard]] UintT OneTimeHashWrapper<UintT>::HashImpl(const char *message, size_t length) const {
        const uint8_t* key = reinterpret_cast<const uint8_t*>(message);
        return one_at_a_time_hash<UintT>(key, length);
    }

    class [[maybe_unused]] SpookyHash16Wrapper final : public BaseHash16Wrapper {
    private:
        [[nodiscard]] uint16_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] SpookyHash24Wrapper final : public BaseHash24Wrapper {
    private:
        [[nodiscard]] uint24_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] SpookyHash32Wrapper final : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] SpookyHash48Wrapper final : public BaseHash48Wrapper {
    private:
        [[nodiscard]] uint48_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] SpookyHash64Wrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };


//------------ MetroHash -----------

    class [[maybe_unused]] MetroHash64_Wrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

//---------- MurmurHashes ---------

    class [[maybe_unused]] MurmurHash1Wrapper final : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] MurmurHash2Wrapper final : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t HashImpl(const char *message, size_t length) const override;
    };
    class [[maybe_unused]] MurmurHash2AWrapper final : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] MurmurHash64AWrapper final : public BaseHash64Wrapper  {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] MurmurHash3Wrapper final : public BaseHash32Wrapper  {
    private:
        [[nodiscard]] uint32_t HashImpl(const char *message, size_t length) const override;
    };

//----------- MUM/mir -----------

    class [[maybe_unused]] MumHashWrapper final : public BaseHash64Wrapper  {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] MirHashWrapper final : public BaseHash64Wrapper  {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

//------------- MX3 --------------

    class [[maybe_unused]] MX3HashWrapper final : public BaseHash64Wrapper  {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

//------------ NMHASH ------------

    class [[maybe_unused]] nmHash32Wrapper final : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] nmHash32XWrapper final : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t HashImpl(const char *message, size_t length) const override;
    };

//--- Paul Hsieh's SuperFastHash ---

    class [[maybe_unused]] SuperFastHashWrapper final : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t HashImpl(const char *message, size_t length) const override;
    };

//---------- PearsonHashes ---------

    class [[maybe_unused]] PearsonHash16 {
    public:
        uint16_t operator()(const char *message, size_t length) const;
        uint16_t operator()(const std::string& message) const;
        void Init() const;
    private:

        mutable std::vector<uint16_t> t16_;
        const uint32_t table_size_ = 65536;
        const uint16_t mask_ = 65535;
    };

    class [[maybe_unused]] PearsonHash16Wrapper final : public BaseHash16Wrapper {
    private:
        uint16_t HashImpl(const char *message, size_t length) const override;

        mutable std::once_flag init_flag_;
        PearsonHash16 hash_;
    };

    class [[maybe_unused]] PearsonHash24 {
    public:
        uint24_t operator()(const char *message, size_t length) const;
        uint24_t operator()(const std::string& message) const;
        void Init() const;

    private:
        mutable std::vector<uint32_t> t12_;
        const uint16_t shift6_ = 6;
        const uint16_t shift12_ = 12;
        const uint32_t table_size_ = 1ull << shift12_;
        const uint32_t bits_mask_ = table_size_ - 1;
        const uint24_t hash_mask_ = 0x020100;

    };

    class [[maybe_unused]] PearsonHash24Wrapper final : public BaseHash24Wrapper {
    private:
        [[nodiscard]] uint24_t HashImpl(const char *message, size_t length) const override;

        mutable std::once_flag init_flag_;
        PearsonHash24 hash_;
    };


    class [[maybe_unused]] PearsonHash32Wrapper final : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t HashImpl(const char *message, size_t length) const override;

        mutable std::once_flag init_flag_;
    };

    class [[maybe_unused]] PearsonHash48 {
    public:
        uint48_t operator()(const char *message, size_t length) const;
        uint48_t operator()(const std::string& message) const;
        void Init() const;

    private:
        uint48_t ROR48(const uint48_t& h) const;

        mutable std::vector<uint32_t> t12_;
        const uint16_t shift6_ = 6;
        const uint16_t shift12_ = 12;
        const uint16_t shift24_ = 24;
        const uint32_t table_size_ = 1ull << shift12_;
        const uint32_t bits_mask_ = table_size_ - 1;
        const uint24_t hash_mask_ = 0x050403020100ull;

    };

    class [[maybe_unused]] PearsonHash48Wrapper final : public BaseHash48Wrapper {
    private:
        [[nodiscard]] uint48_t HashImpl(const char *message, size_t length) const override;

        mutable std::once_flag init_flag_;
        PearsonHash48 hash_;
    };

    class [[maybe_unused]] PearsonHash64Wrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;

        mutable std::once_flag init_flag_;
    };

//----------- PengyHash ------------

    class [[maybe_unused]] PengyHash64Wrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

//------------ PJW Hash ------------

    template<UnsignedIntegral UintT>
    class [[maybe_unused]] PJWHashWrapper final : public detail::BaseHashWrapper<UintT> {
    private:
        [[nodiscard]] UintT HashImpl(const char *message, size_t length) const override;
    };

    template<UnsignedIntegral UintT>
    [[nodiscard]] UintT PJWHashWrapper<UintT>::HashImpl(const char *message, size_t length) const {
        return PJWHash<UintT>(message, length);
    }

    template<>
    class [[maybe_unused]] PJWHashWrapper<uint24_t> final : public detail::BaseHashWrapper<uint24_t> {
    private:
        [[nodiscard]] uint24_t HashImpl(const char *message, size_t length) const override {
            return PJWHash<uint24_t, 24>(message, length);
        }
    };

    template<>
    class [[maybe_unused]] PJWHashWrapper<uint48_t> final : public detail::BaseHashWrapper<uint48_t> {
    private:
        [[nodiscard]] uint48_t HashImpl(const char *message, size_t length) const override {
            return PJWHash<uint48_t, 48>(message, length);
        }
    };

//-------------- SDBM --------------

    template<UnsignedIntegral UintT>
    class [[maybe_unused]] SDBMHashWrapper final : public detail::BaseHashWrapper<UintT> {
    private:
        [[nodiscard]] UintT HashImpl(const char *message, size_t length) const override;
    };

    template<UnsignedIntegral UintT>
    [[nodiscard]] UintT SDBMHashWrapper<UintT>::HashImpl(const char *message, size_t length) const {
        return SDBMHash<UintT>(message, length);
    }

//------------- SipHash ------------

    class [[maybe_unused]] SipHashWrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };


    class [[maybe_unused]] SipHash13Wrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] SipHashAVX2Wrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] SipHash13AVX2Wrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] HalfSipHashWrapper final : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t HashImpl(const char *message, size_t length) const override;
    };

//-------------- T1HA --------------

    class [[maybe_unused]] T1HA0_32leWrapper final : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] T1HA0_32beWrapper final : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] T1HA0_AVX2_Wrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] T1HA1LeWrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] T1HA1BeWrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };


    class [[maybe_unused]] T1HA2AtonceWrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

//------------ wyHashes -----------
//https://github.com/wangyi-fudan/wyhash

    class [[maybe_unused]] wyHash32Wrapper final : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] wyHash64Wrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

//------------ xxHashes -----------

    class [[maybe_unused]] xxHash32Wrapper final : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] xxHash64Wrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] XXH3_64BitsWrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };

    class [[maybe_unused]] XXH3_64bits_withSeedWrapper final : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t HashImpl(const char *message, size_t length) const override;
    };
}

#endif //THESIS_WORK_HASH_WRAPPERS_H
