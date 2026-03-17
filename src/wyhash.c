#ifndef WYHASH_STRIPPED_H
#define WYHASH_STRIPPED_H

#include <stdint.h>
#include <string.h>

#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
  #define _likely_(x)  __builtin_expect(x,1)
  #define _unlikely_(x)  __builtin_expect(x,0)
#else
  #define _likely_(x) (x)
  #define _unlikely_(x) (x)
#endif

static inline uint64_t _wyrot(uint64_t x) { return (x >> 32) | (x << 32); }

static inline void _wymum(uint64_t *A, uint64_t *B){
    __uint128_t r = *A; r *= *B;
    *A = (uint64_t)r; 
    *B = (uint64_t)(r >> 64);
}

static inline uint64_t _wymix(uint64_t A, uint64_t B){ _wymum(&A,&B); return A^B; }

static inline uint64_t _wyr8(const uint8_t *p) { uint64_t v; memcpy(&v, p, 8); return v; }
static inline uint64_t _wyr4(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return v; }
static inline uint64_t _wyr3(const uint8_t *p, size_t k) { 
    return (((uint64_t)p[0])<<16)|(((uint64_t)p[k>>1])<<8)|p[k-1]; 
}

static const uint64_t _wyp[4] = {
    0x2d358dccaa6c78a5ULL,
    0x8bb84b93962eacc9ULL,
    0x4b33a62ed433d4a3ULL,
    0x4d5a2da51de1aa47ULL
};

static inline uint64_t wyhash(const void *key, size_t len, uint64_t seed){
    const uint8_t *p=(const uint8_t*)key; 
    seed ^= _wymix(seed ^ _wyp[0], _wyp[1]);
    uint64_t a, b;

    if(_likely_(len <= 16)){
        if(_likely_(len >= 4)){
            a = (_wyr4(p)<<32)|_wyr4(p + ((len>>3)<<2));
            b = (_wyr4(p + len - 4)<<32)|_wyr4(p + len - 4 - ((len>>3)<<2));
        } else if(_likely_(len > 0)){
            a = _wyr3(p, len); b = 0;
        } else a = b = 0;
    } else {
        size_t i = len;
        if(_unlikely_(i >= 48)){
            uint64_t see1=seed, see2=seed;
            do {
                seed = _wymix(_wyr8(p)^_wyp[1], _wyr8(p+8)^seed);
                see1 = _wymix(_wyr8(p+16)^_wyp[2], _wyr8(p+24)^see1);
                see2 = _wymix(_wyr8(p+32)^_wyp[3], _wyr8(p+40)^see2);
                p += 48; i -= 48;
            } while(_likely_(i >= 48));
            seed ^= see1 ^ see2;
        }
        while(_unlikely_(i > 16)){ seed = _wymix(_wyr8(p)^_wyp[1], _wyr8(p+8)^seed); i -= 16; p += 16; }
        a = _wyr8(p + i - 16); b = _wyr8(p + i - 8);
    }

    a ^= _wyp[1]; b ^= seed; _wymum(&a, &b);
    return _wymix(a ^ _wyp[0] ^ len, b ^ _wyp[1]);
}

#endif
