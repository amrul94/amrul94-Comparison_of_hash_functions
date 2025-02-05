#ifndef FASTHASH_INLINE_H
#define FASTHASH_INLINE_H

#include <cstdint>
#include <cstdio>

// Compression function for Merkle-Damgard construction.
// This function is generated using the framework provided.
#define mix(h) ({					\
			(h) ^= (h) >> 23;		\
			(h) *= 0x2127599bf4325c37ULL;	\
			(h) ^= (h) >> 47; })


namespace fasthash_inline {
    /**
     * fasthash64 - 64-bit implementation of fasthash
     * @buf:  data buffer
     * @len:  data size
     * @seed: the seed
     */
    static inline uint64_t fasthash64(const void *buf, size_t len, uint64_t seed) {
        const uint64_t    m = 0x880355f21e6d1965ULL;
        const uint64_t *pos = (const uint64_t *)buf;
        const uint64_t *end = pos + (len / 8);
        const unsigned char *pos2;
        uint64_t h = seed ^ (len * m);
        uint64_t v;

        while (pos != end) {
            v  = *pos++;
            h ^= mix(v);
            h *= m;
        }

        pos2 = (const unsigned char*)pos;
        v = 0;

        switch (len & 7) {
            case 7: v ^= (uint64_t)pos2[6] << 48;
            case 6: v ^= (uint64_t)pos2[5] << 40;
            case 5: v ^= (uint64_t)pos2[4] << 32;
            case 4: v ^= (uint64_t)pos2[3] << 24;
            case 3: v ^= (uint64_t)pos2[2] << 16;
            case 2: v ^= (uint64_t)pos2[1] << 8;
            case 1: v ^= (uint64_t)pos2[0];
                h ^= mix(v);
                h *= m;
        }

        return mix(h);
    }


    /**
     * fasthash32 - 32-bit implementation of fasthash
     * @buf:  data buffer
     * @len:  data size
     * @seed: the seed
     */
    static inline uint32_t fasthash32(const void *buf, size_t len, uint32_t seed) {
        // the following trick converts the 64-bit hashcode to Fermat
        // residue, which shall retain information from both the higher
        // and lower parts of hashcode.
        uint64_t h = fasthash64(buf, len, seed);
        return h - (h >> 32);
    }
}

#undef mix

#endif //FASTHASH_INLINE_H
