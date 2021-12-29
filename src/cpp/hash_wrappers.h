#ifndef THESIS_WORK_HASH_WRAPPERS_H
#define THESIS_WORK_HASH_WRAPPERS_H

#include <array>
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

#include "img.h"


// HFL = Hash function library
namespace hfl {

    using uint24_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<24, 24,
            boost::multiprecision::unsigned_magnitude,
            boost::multiprecision::unchecked, void>>;

    using uint48_t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<48, 48,
            boost::multiprecision::unsigned_magnitude,
            boost::multiprecision::unchecked, void>>;


    class [[maybe_unused]] bitfield24 {
    private:
        uint32_t value : 24;
    };


    std::string ReadFile(std::ifstream& file);

    namespace detail {
        template<class Type>
        std::string WriteToString(Type source) {
            auto size = sizeof(Type);
            std::string str;
            str.resize(size);
            memcpy(str.data(), &source, size);
            return str;
        }

        template<typename UintT>
        class BaseHashWrapper {
        public:
            UintT operator()(std::string_view str) const {
                return Hash(str);
            }

            UintT operator()(std::ifstream& file) const {
                std::string binary_file = ReadFile(file);
                assert(!binary_file.empty());
                return Hash(binary_file);
            }

            UintT operator()(const img::Image& image) const {

                // Возможно стоит сделать так:
                // const char* bytes = reinterpret_cast<const char*>(image.GetLine(0);
                // const size_t size = image.GetHeight() * image.GetWidth();
                // const std::string_view str(bytes, size);
                // return Hash(str);

                const char* bytes = reinterpret_cast<const char*>(image.GetLine(0));
                return Hash(bytes);
            }

            UintT operator()(int8_t number) const {
                uint8_t number8 = number;
                uint64_t number64 = number8;
                return operator()(number64);
            }

            UintT operator()(int16_t number) const {
                uint64_t number64 = number;
                return operator()(number64);
            }

            UintT operator()(int32_t number) const {
                uint64_t number64 = number;
                return operator()(number64);
            }

            UintT operator()(int64_t number) const {
                uint64_t number64 = number;
                return operator()(number64);
            }


            UintT operator()(uint8_t number) const {
                uint64_t number64 = number;
                return operator()(number64);
            }

            UintT operator()(uint16_t number) const {
                uint64_t number64 = number;
                return operator()(number64);
            }

            UintT operator()(uint32_t number) const {
                uint64_t number64 = number;
                return operator()(number64);
            }

            UintT operator()(uint64_t number) const {
                std::string bytes = WriteToString<uint64_t>(number);
                assert(sizeof(bytes[0]) == 1);
                assert((&bytes[1] - &bytes[0]) == 1);
                return Hash(bytes);
            }

            virtual ~BaseHashWrapper() = default;

        private:
            [[nodiscard]] virtual UintT Hash(std::string_view str) const = 0;
        };
    }


    using BaseHash16Wrapper = detail::BaseHashWrapper<uint16_t>;
    using BaseHash24Wrapper = detail::BaseHashWrapper<uint24_t>;
    using BaseHash32Wrapper = detail::BaseHashWrapper<uint32_t>;
    using BaseHash48Wrapper = detail::BaseHashWrapper<uint48_t>;
    using BaseHash64Wrapper = detail::BaseHashWrapper<uint64_t>;

//----- Bernstein's hash DJB2 ------

    template<typename UintT>
    class [[maybe_unused]] DJB2HashWrapper : public detail::BaseHashWrapper<UintT> {
    private:
        [[nodiscard]] UintT Hash(std::string_view str) const override {
            return DJB2Hash<UintT>(str);
        }
    };


//----- Rolling Hash (BuzHash) -----

    template<typename UintT>
    class [[maybe_unused]] BuzHashWrapper : public detail::BaseHashWrapper<UintT> {
    public:
        BuzHashWrapper() noexcept = default;

    private:
        UintT Hash(std::string_view str) const override {
            std::scoped_lock guard{hash_mutex_};
            return hasher_.hash(str);
        }

        mutable std::mutex hash_mutex_;
        mutable CyclicHash<UintT> hasher_{4096, sizeof(UintT) * 8};
    };

    template<>
    class [[maybe_unused]] BuzHashWrapper<uint24_t> : public detail::BaseHashWrapper<uint24_t> {
    public:
        BuzHashWrapper() noexcept = default;

    private:
        uint24_t Hash(std::string_view str) const override {
            std::scoped_lock guard{hash_mutex_};
            return hasher_.hash(str);
        }

        mutable std::mutex hash_mutex_;
        mutable CyclicHash<uint32_t> hasher_{4096, 24};
    };

    template<>
    class [[maybe_unused]] BuzHashWrapper<uint48_t> : public BaseHash48Wrapper {
    public:
        BuzHashWrapper() noexcept = default;

    private:
        uint48_t Hash(std::string_view str) const override {
            std::scoped_lock guard{hash_mutex_};
            return hasher_.hash(str);
        }

        mutable std::mutex hash_mutex_;
        mutable CyclicHash<uint64_t> hasher_{4096, 48};
    };

//----------- CityHashes ----------

    class [[maybe_unused]] CityHash32Wrapper : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] CityHash64Wrapper : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t Hash(std::string_view str) const override;
    };

//----------- FarmHashes ----------

    class [[maybe_unused]] FarmHash32Wrapper : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] FarmHash64Wrapper : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t Hash(std::string_view str) const override;
    };

//------------ FastHash ------------

    class [[maybe_unused]] FastHash16Wrapper : public BaseHash16Wrapper {
    private:
        [[nodiscard]] uint16_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] FastHash24Wrapper : public BaseHash24Wrapper {
    private:
        [[nodiscard]] uint24_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] FastHash32Wrapper : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] FastHash48Wrapper : public BaseHash48Wrapper {
    private:
        [[nodiscard]] uint48_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] FastHash64Wrapper : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t Hash(std::string_view str) const override;
    };

//---------- FNV-1a hash -----------

    class [[maybe_unused]] FNV1aHash16Wrapper : public BaseHash16Wrapper {
    private:
        [[nodiscard]] uint16_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] FNV1aHash24Wrapper : public BaseHash24Wrapper {
    private:
        [[nodiscard]] uint24_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] FNV1aHash32Wrapper : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] FNV1aHash48Wrapper : public BaseHash48Wrapper {
    private:
        [[nodiscard]] uint48_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] FNV1aHash64Wrapper : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t Hash(std::string_view str) const override;
    };


//--------- Jenkins hash -----------

    template<typename UintT>
    class [[maybe_unused]] OneTimeHashWrapper : public detail::BaseHashWrapper<UintT> {
    private:
        [[nodiscard]] UintT Hash(std::string_view str) const override {
            return one_at_a_time_hash<UintT>(reinterpret_cast<const uint8_t*>(str.data()), str.size());
        }
    };

    class [[maybe_unused]] Lookup3LittleWrapper : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] Lookup3BigWrapper : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] SpookyHash16Wrapper : public BaseHash16Wrapper {
    private:
        [[nodiscard]] uint16_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] SpookyHash24Wrapper : public BaseHash24Wrapper {
    private:
        [[nodiscard]] uint24_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] SpookyHash32Wrapper : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] SpookyHash48Wrapper : public BaseHash48Wrapper {
    private:
        [[nodiscard]] uint48_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] SpookyHash64Wrapper : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t Hash(std::string_view str) const override;
    };


//------------ MetroHash -----------

    class [[maybe_unused]] MetroHash64_Wrapper : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t Hash(std::string_view str) const override;
    };

//---------- MurmurHashes ---------

    class [[maybe_unused]] MurmurHash1Wrapper : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] MurmurHash2Wrapper : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t Hash(std::string_view str) const override;
    };
    class [[maybe_unused]] MurmurHash2AWrapper : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] MurmurHash64AWrapper: public BaseHash64Wrapper  {
    private:
        [[nodiscard]] uint64_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] MurmurHash3Wrapper: public BaseHash32Wrapper  {
    private:
        [[nodiscard]] uint32_t Hash(std::string_view str) const override;
    };

//--- Paul Hsieh's SuperFastHash ---

    class [[maybe_unused]] SuperFastHashWrapper : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t Hash(std::string_view str) const override;
    };

//---------- PearsonHashes ---------

    class [[maybe_unused]] PearsonHash16Wrapper : public BaseHash16Wrapper {
    private:
        void PearsonHashInit() const;
        [[nodiscard]] uint16_t Hash(std::string_view str) const override;

        mutable std::once_flag init_flag_;
        mutable std::vector<uint16_t> t16_;
        const uint32_t table_size_ = 65536;
        const uint32_t mask_ = 65535;
    };

    class [[maybe_unused]] PearsonHash24Wrapper : public BaseHash24Wrapper {

    private:
        void PearsonHashInit() const;
        [[nodiscard]] uint24_t Hash(std::string_view str) const override;


        mutable std::once_flag init_flag_;
        mutable std::vector<uint32_t> t24_;
        const uint32_t table_size_ = 16'777'216;
        const uint32_t mask_ = 16'777'215;

    };
    class [[maybe_unused]] PearsonHash32Wrapper : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t Hash(std::string_view str) const override;

        mutable std::once_flag init_flag;
    };

    class [[maybe_unused]] PearsonHash64Wrapper : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t Hash(std::string_view str) const override;

        mutable std::once_flag init_flag;
    };

//----------- PengyHash ------------

    class [[maybe_unused]] PengyHash64Wrapper : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t Hash(std::string_view str) const override;
    };

//------------ PJW Hash ------------

    template<typename UintT>
    class [[maybe_unused]] PJWHashWrapper : public detail::BaseHashWrapper<UintT> {
    private:
        [[nodiscard]] UintT Hash(std::string_view str) const override {
            return PJWHash<UintT>(str);
        }
    };

    template<>
    class [[maybe_unused]] PJWHashWrapper<uint24_t>: public detail::BaseHashWrapper<uint24_t> {
    private:
        [[nodiscard]] uint24_t Hash(std::string_view str) const override {
            return PJWHash<uint24_t, 24>(str);
        }
    };

    template<>
    class [[maybe_unused]] PJWHashWrapper<uint48_t>: public detail::BaseHashWrapper<uint48_t> {
    private:
        [[nodiscard]] uint48_t Hash(std::string_view str) const override {
            return PJWHash<uint48_t, 48>(str);
        }
    };

//-------------- SDBM --------------

    template<typename UintT>
    class [[maybe_unused]] SDBMHashWrapper : public detail::BaseHashWrapper<UintT> {
    private:
        [[nodiscard]] UintT Hash(std::string_view str) const override {
            return SDBMHash<UintT>(str);
        }
    };

//-------------- T1HA --------------

    class [[maybe_unused]] T1HA0_32leWrapper : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] T1HA0_32beWrapper : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] T1HA1LeWrapper : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] T1HA1BeWrapper : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t Hash(std::string_view str) const override;
    };


    class [[maybe_unused]] T1HA2AtonceWrapper : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t Hash(std::string_view str) const override;
    };

//------------ wyHashes -----------
//https://github.com/wangyi-fudan/wyhash

    class [[maybe_unused]] WyHash32Wrapper : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] WyHash64Wrapper : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t Hash(std::string_view str) const override;
    };

//------------ xxHashes -----------

    class [[maybe_unused]] xxHash32Wrapper : public BaseHash32Wrapper {
    private:
        [[nodiscard]] uint32_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] xxHash64Wrapper : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] XXH3_64BitsWrapper : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t Hash(std::string_view str) const override;
    };

    class [[maybe_unused]] XXH3_64BitsWithSeedWrapper : public BaseHash64Wrapper {
    private:
        [[nodiscard]] uint64_t Hash(std::string_view str) const override;
    };
}

#endif //THESIS_WORK_HASH_WRAPPERS_H