/*
 * SpookyHash - 128-bit noncryptographic hash function
 *
 * Written in 2012 by Bob Jenkins
 *
 * Converted to C in 2015 by Joergen Ibsen
 *
 * To the extent possible under law, the author(s) have dedicated all
 * copyright and related and neighboring rights to this software to the
 * public domain worldwide. This software is distributed without any
 * warranty. <http://creativecommons.org/publicdomain/zero/1.0/>
 *
 * Original comment from SpookyV2.h by Bob Jenkins:
 *
 * SpookyHash: a 128-bit noncryptographic hash function
 * By Bob Jenkins, public domain
 *   Oct 31 2010: alpha, framework + SpookyHash::Mix appears right
 *   Oct 31 2011: alpha again, Mix only good to 2^^69 but rest appears right
 *   Dec 31 2011: beta, improved Mix, tested it for 2-bit deltas
 *   Feb  2 2012: production, same bits as beta
 *   Feb  5 2012: adjusted definitions of uint* to be more portable
 *   Mar 30 2012: 3 bytes/cycle, not 4.  Alpha was 4 but wasn't thorough enough.
 *   August 5 2012: SpookyV2 (different results)
 *
 * Up to 3 bytes/cycle for long messages.  Reasonably fast for short messages.
 * All 1 or 2 bit deltas achieve avalanche within 1% bias per output bit.
 *
 * This was developed for and tested on 64-bit x86-compatible processors.
 * It assumes the processor is little-endian.  There is a macro
 * controlling whether unaligned reads are allowed (by default they are).
 * This should be an equally good hash on big-endian machines, but it will
 * compute different results on them than on little-endian machines.
 *
 * Google's CityHash has similar specs to SpookyHash, and CityHash is faster
 * on new Intel boxes.  MD4 and MD5 also have similar specs, but they are orders
 * of magnitude slower.  CRCs are two or more times slower, but unlike
 * SpookyHash, they have nice math for combining the CRCs of pieces to form
 * the CRCs of wholes.  There are also cryptographic hashes, but those are even
 * slower than MD5.
 */

#ifndef SPOOKY_INLINE_H
#define SPOOKY_INLINE_H

#include <memory.h>
#include <cstddef>
#include <cstdint>

#define restrict __restrict

#ifndef ROTL64
#   define ROTL64(x, k) (((x) << (k)) | ((x) >> (64 - (k))))
#endif

namespace spooky_inline {

    // number of uint64_t's in internal state
    inline constexpr uint64_t SC_NUMVARS = 12U;

    // size of the internal state
    inline constexpr uint64_t SC_BLOCKSIZE = SC_NUMVARS * 8U;

    // size of buffer of unhashed data, in bytes
    inline constexpr uint64_t SC_BUFSIZE = 2U * SC_BLOCKSIZE;

    inline constexpr uint8_t ALLOW_UNALIGNED_READS = 1;

    inline constexpr uint64_t SC_CONST = 0xDEADBEEFDEADBEEFULL;

    static inline bool
    spooky_is_aligned(const void *p, size_t size)
    {
        return (uintptr_t) p % size == 0;
    }


    static inline bool
    spooky_is_little_endian()
    {
        const union {
            uint32_t i;
            uint8_t c[sizeof(uint32_t)];
        } x = { 1 };

        return x.c[0];
    }

    //
    // Read uint64_t in little-endian order.
    //
    static inline uint64_t
    spooky_read_le64(const uint64_t *s)
    {
        if (spooky_is_little_endian()) {
            uint64_t v;
            memcpy(&v, s, sizeof(v));
            return v;
        }
        else {
            const auto *p = (const uint8_t *) s;
            return (uint64_t) p[0]
                   | ((uint64_t) p[1] << 8)
                   | ((uint64_t) p[2] << 16)
                   | ((uint64_t) p[3] << 24)
                   | ((uint64_t) p[4] << 32)
                   | ((uint64_t) p[5] << 40)
                   | ((uint64_t) p[6] << 48)
                   | ((uint64_t) p[7] << 56);
        }
    }


    //
    // The goal is for each bit of the input to expand into 128 bits of
    //   apparent entropy before it is fully overwritten.
    // n trials both set and cleared at least m bits of h0 h1 h2 h3
    //   n: 2   m: 29
    //   n: 3   m: 46
    //   n: 4   m: 57
    //   n: 5   m: 107
    //   n: 6   m: 146
    //   n: 7   m: 152
    // when run forwards or backwards
    // for all 1-bit and 2-bit diffs
    // with diffs defined by either xor or subtraction
    // with a base of all zeros plus a counter, or plus another bit, or random
    //
    static inline void
    spooky_short_mix(uint64_t *h)
    {
        h[2] = ROTL64(h[2], 50);  h[2] += h[3];  h[0] ^= h[2];
        h[3] = ROTL64(h[3], 52);  h[3] += h[0];  h[1] ^= h[3];
        h[0] = ROTL64(h[0], 30);  h[0] += h[1];  h[2] ^= h[0];
        h[1] = ROTL64(h[1], 41);  h[1] += h[2];  h[3] ^= h[1];
        h[2] = ROTL64(h[2], 54);  h[2] += h[3];  h[0] ^= h[2];
        h[3] = ROTL64(h[3], 48);  h[3] += h[0];  h[1] ^= h[3];
        h[0] = ROTL64(h[0], 38);  h[0] += h[1];  h[2] ^= h[0];
        h[1] = ROTL64(h[1], 37);  h[1] += h[2];  h[3] ^= h[1];
        h[2] = ROTL64(h[2], 62);  h[2] += h[3];  h[0] ^= h[2];
        h[3] = ROTL64(h[3], 34);  h[3] += h[0];  h[1] ^= h[3];
        h[0] = ROTL64(h[0], 5);   h[0] += h[1];  h[2] ^= h[0];
        h[1] = ROTL64(h[1], 36);  h[1] += h[2];  h[3] ^= h[1];
    }


    //
    // Mix all 4 inputs together so that h0, h1 are a hash of them all.
    //
    // For two inputs differing in just the input bits
    // Where "differ" means xor or subtraction
    // And the base value is random, or a counting value starting at that bit
    // The final result will have each bit of h0, h1 flip
    // For every input bit,
    // with probability 50 +- .3% (it is probably better than that)
    // For every pair of input bits,
    // with probability 50 +- .75% (the worst case is approximately that)
    //
    static inline void
    spooky_short_end(uint64_t *h)
    {
        h[3] ^= h[2];  h[2] = ROTL64(h[2], 15);  h[3] += h[2];
        h[0] ^= h[3];  h[3] = ROTL64(h[3], 52);  h[0] += h[3];
        h[1] ^= h[0];  h[0] = ROTL64(h[0], 26);  h[1] += h[0];
        h[2] ^= h[1];  h[1] = ROTL64(h[1], 51);  h[2] += h[1];
        h[3] ^= h[2];  h[2] = ROTL64(h[2], 28);  h[3] += h[2];
        h[0] ^= h[3];  h[3] = ROTL64(h[3], 9);   h[0] += h[3];
        h[1] ^= h[0];  h[0] = ROTL64(h[0], 47);  h[1] += h[0];
        h[2] ^= h[1];  h[1] = ROTL64(h[1], 54);  h[2] += h[1];
        h[3] ^= h[2];  h[2] = ROTL64(h[2], 32);  h[3] += h[2];
        h[0] ^= h[3];  h[3] = ROTL64(h[3], 25);  h[0] += h[3];
        h[1] ^= h[0];  h[0] = ROTL64(h[0], 63);  h[1] += h[0];
    }

    //
    // short hash ... it could be used on any message,
    // but it's used by Spooky just for short messages.
    //
    static inline void
    spooky_short(const void *restrict message, size_t length,
                 uint64_t *restrict hash1, uint64_t *restrict hash2)
    {
        uint64_t buf[2 * SC_NUMVARS];
        union {
            const uint8_t *p8;
            uint64_t *p64;
        } u{};

        u.p8 = (const uint8_t *) message;

        if (ALLOW_UNALIGNED_READS == 0 && !spooky_is_aligned(u.p8, 8)) {
            memcpy(buf, message, length);
            u.p64 = buf;
        }

        size_t left = length % 32;
        uint64_t h[4];
        h[0] = *hash1;
        h[1] = *hash2;
        h[2] = SC_CONST;
        h[3] = SC_CONST;

        if (length > 15) {
            const uint64_t *end = u.p64 + (length / 32) * 4;

            // handle all complete sets of 32 bytes
            for (; u.p64 < end; u.p64 += 4) {
                h[2] += spooky_read_le64(&u.p64[0]);
                h[3] += spooky_read_le64(&u.p64[1]);
                spooky_short_mix(h);
                h[0] += spooky_read_le64(&u.p64[2]);
                h[1] += spooky_read_le64(&u.p64[3]);
            }

            //Handle the case of 16+ remaining bytes.
            if (left >= 16) {
                h[2] += spooky_read_le64(&u.p64[0]);
                h[3] += spooky_read_le64(&u.p64[1]);
                spooky_short_mix(h);
                u.p64 += 2;
                left -= 16;
            }
        }

        // Handle the last 0..15 bytes, and its length
        h[3] += ((uint64_t) length) << 56;
        switch (left) {
            case 15:
                h[3] += ((uint64_t) u.p8[14]) << 48;
            case 14:
                h[3] += ((uint64_t) u.p8[13]) << 40;
            case 13:
                h[3] += ((uint64_t) u.p8[12]) << 32;
            case 12:
                h[3] += ((uint64_t) u.p8[11]) << 24;
            case 11:
                h[3] += ((uint64_t) u.p8[10]) << 16;
            case 10:
                h[3] += ((uint64_t) u.p8[9]) << 8;
            case 9:
                h[3] += (uint64_t) u.p8[8];
            case 8:
                h[2] += spooky_read_le64(&u.p64[0]);
                break;
            case 7:
                h[2] += ((uint64_t) u.p8[6]) << 48;
            case 6:
                h[2] += ((uint64_t) u.p8[5]) << 40;
            case 5:
                h[2] += ((uint64_t) u.p8[4]) << 32;
            case 4:
                h[2] += ((uint64_t) u.p8[3]) << 24;
            case 3:
                h[2] += ((uint64_t) u.p8[2]) << 16;
            case 2:
                h[2] += ((uint64_t) u.p8[1]) << 8;
            case 1:
                h[2] += (uint64_t) u.p8[0];
                break;
            case 0:
                h[2] += SC_CONST;
                h[3] += SC_CONST;
        }
        spooky_short_end(h);
        *hash1 = h[0];
        *hash2 = h[1];
    }


    //
    // This is used if the input is 96 bytes long or longer.
    //
    // The internal state is fully overwritten every 96 bytes.
    // Every input bit appears to cause at least 128 bits of entropy
    // before 96 other bytes are combined, when run forward or backward
    //   For every input bit,
    //   Two inputs differing in just that input bit
    //   Where "differ" means xor or subtraction
    //   And the base value is random
    //   When run forward or backwards one Mix
    // I tried 3 pairs of each; they all differed by at least 212 bits.
    //
    static inline void
    spooky_mix(const uint64_t *restrict data, uint64_t *restrict s)
    {
        s[0] += spooky_read_le64(&data[0]);          s[2] ^= s[10];
        s[11] ^= s[0];   s[0] = ROTL64(s[0], 11);    s[11] += s[1];
        s[1] += spooky_read_le64(&data[1]);          s[3] ^= s[11];
        s[0] ^= s[1];    s[1] = ROTL64(s[1], 32);    s[0] += s[2];
        s[2] += spooky_read_le64(&data[2]);          s[4] ^= s[0];
        s[1] ^= s[2];    s[2] = ROTL64(s[2], 43);    s[1] += s[3];
        s[3] += spooky_read_le64(&data[3]);          s[5] ^= s[1];
        s[2] ^= s[3];    s[3] = ROTL64(s[3], 31);    s[2] += s[4];
        s[4] += spooky_read_le64(&data[4]);          s[6] ^= s[2];
        s[3] ^= s[4];    s[4] = ROTL64(s[4], 17);    s[3] += s[5];
        s[5] += spooky_read_le64(&data[5]);          s[7] ^= s[3];
        s[4] ^= s[5];    s[5] = ROTL64(s[5], 28);    s[4] += s[6];
        s[6] += spooky_read_le64(&data[6]);          s[8] ^= s[4];
        s[5] ^= s[6];    s[6] = ROTL64(s[6], 39);    s[5] += s[7];
        s[7] += spooky_read_le64(&data[7]);          s[9] ^= s[5];
        s[6] ^= s[7];    s[7] = ROTL64(s[7], 57);    s[6] += s[8];
        s[8] += spooky_read_le64(&data[8]);          s[10] ^= s[6];
        s[7] ^= s[8];    s[8] = ROTL64(s[8], 55);    s[7] += s[9];
        s[9] += spooky_read_le64(&data[9]);          s[11] ^= s[7];
        s[8] ^= s[9];    s[9] = ROTL64(s[9], 54);    s[8] += s[10];
        s[10] += spooky_read_le64(&data[10]);        s[0] ^= s[8];
        s[9] ^= s[10];   s[10] = ROTL64(s[10], 22);  s[9] += s[11];
        s[11] += spooky_read_le64(&data[11]);        s[1] ^= s[9];
        s[10] ^= s[11];  s[11] = ROTL64(s[11], 46);  s[10] += s[0];
    }

    //
    // Mix all 12 inputs together so that h0, h1 are a hash of them all.
    //
    // For two inputs differing in just the input bits
    // Where "differ" means xor or subtraction
    // And the base value is random, or a counting value starting at that bit
    // The final result will have each bit of h0, h1 flip
    // For every input bit,
    // with probability 50 +- .3%
    // For every pair of input bits,
    // with probability 50 +- 3%
    //
    // This does not rely on the last Mix() call having already mixed some.
    // Two iterations was almost good enough for a 64-bit result, but a
    // 128-bit result is reported, so End() does three iterations.
    //
    static inline void
    spooky_end_partial(uint64_t *h)
    {
        h[11] += h[1];  h[2] ^= h[11];  h[1] = ROTL64(h[1], 44);
        h[0] += h[2];   h[3] ^= h[0];   h[2] = ROTL64(h[2], 15);
        h[1] += h[3];   h[4] ^= h[1];   h[3] = ROTL64(h[3], 34);
        h[2] += h[4];   h[5] ^= h[2];   h[4] = ROTL64(h[4], 21);
        h[3] += h[5];   h[6] ^= h[3];   h[5] = ROTL64(h[5], 38);
        h[4] += h[6];   h[7] ^= h[4];   h[6] = ROTL64(h[6], 33);
        h[5] += h[7];   h[8] ^= h[5];   h[7] = ROTL64(h[7], 10);
        h[6] += h[8];   h[9] ^= h[6];   h[8] = ROTL64(h[8], 13);
        h[7] += h[9];   h[10] ^= h[7];  h[9] = ROTL64(h[9], 38);
        h[8] += h[10];  h[11] ^= h[8];  h[10] = ROTL64(h[10], 53);
        h[9] += h[11];  h[0] ^= h[9];   h[11] = ROTL64(h[11], 42);
        h[10] += h[0];  h[1] ^= h[10];  h[0] = ROTL64(h[0], 54);
    }

    static inline void
    spooky_end(const uint64_t *restrict data, uint64_t *restrict h)
    {
        h[0] += spooky_read_le64(&data[0]);
        h[1] += spooky_read_le64(&data[1]);
        h[2] += spooky_read_le64(&data[2]);
        h[3] += spooky_read_le64(&data[3]);
        h[4] += spooky_read_le64(&data[4]);
        h[5] += spooky_read_le64(&data[5]);
        h[6] += spooky_read_le64(&data[6]);
        h[7] += spooky_read_le64(&data[7]);
        h[8] += spooky_read_le64(&data[8]);
        h[9] += spooky_read_le64(&data[9]);
        h[10] += spooky_read_le64(&data[10]);
        h[11] += spooky_read_le64(&data[11]);
        spooky_end_partial(h);
        spooky_end_partial(h);
        spooky_end_partial(h);
    }

    // do the whole hash in one call
    static inline void
    spooky_hash128(const void *restrict message, size_t length,
                   uint64_t *restrict hash1, uint64_t *restrict hash2)
    {
        if (length < SC_BUFSIZE) {
            spooky_short(message, length, hash1, hash2);
            return;
        }

        uint64_t h[SC_NUMVARS];
        uint64_t buf[SC_NUMVARS];
        uint64_t *end;
        union {
            const uint8_t *p8;
            uint64_t *p64;
        } u{};
        size_t left;

        h[0] = h[3] = h[6] = h[9] = *hash1;
        h[1] = h[4] = h[7] = h[10] = *hash2;
        h[2] = h[5] = h[8] = h[11] = SC_CONST;

        u.p8 = (const uint8_t *) message;
        end = u.p64 + (length / SC_BLOCKSIZE) * SC_NUMVARS;

        // handle all whole SC_BLOCKSIZE blocks of bytes
        if (ALLOW_UNALIGNED_READS || spooky_is_aligned(u.p8, 8)) {
            do {
                spooky_mix(u.p64, h);
                u.p64 += SC_NUMVARS;
            } while (u.p64 < end);
        }
        else {
            do {
                memcpy(buf, u.p64, SC_BLOCKSIZE);
                spooky_mix(buf, h);
                u.p64 += SC_NUMVARS;
            } while (u.p64 < end);
        }

        // handle the last partial block of SC_BLOCKSIZE bytes
        left = length - ((const uint8_t *) end - (const uint8_t *) message);
        memcpy(buf, end, left);
        memset(((uint8_t *) buf) + left, 0, SC_BLOCKSIZE - left);
        ((uint8_t *) buf)[SC_BLOCKSIZE - 1] = (uint8_t) left;

        // do some final mixing
        spooky_end(buf, h);
        *hash1 = h[0];
        *hash2 = h[1];
    }

    static inline uint64_t
    spooky_hash64(const void *message, size_t length, uint64_t seed)
    {
        uint64_t hash1 = seed;
        spooky_hash128(message, length, &hash1, &seed);
        return hash1;
    }


    static inline uint32_t
    spooky_hash32(const void *message, size_t length, uint32_t seed)
    {
        uint64_t hash1 = seed, hash2 = seed;
        spooky_hash128(message, length, &hash1, &hash2);
        return (uint32_t) hash1;
    }
}

#undef restrict
#undef ROTL64

#endif //SPOOKY_INLINE_H