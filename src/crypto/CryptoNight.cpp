/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2016-2017 XMRig       <support@xmrig.com>
 *
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

//#include <iostream>
//#include <boost/chrono.hpp>

#include "crypto/CryptoNight.h"

#if defined(XMRIG_ARM)
#   include "crypto/CryptoNight_arm.h"
#else
#   include "crypto/CryptoNight_x86.h"
#endif

#include "crypto/CryptoNight_test.h"
#include "Options.h"

template <size_t NUM_HASH_BLOCKS>
static void cryptonight_aesni(const void *input, size_t size, void *output, cryptonight_ctx *ctx) {
#   if !defined(XMRIG_ARMv7)
  cryptonight_multi_hash<0x80000, MEMORY, 0x1FFFF0, false, NUM_HASH_BLOCKS>(input, size, output, ctx);
#   endif
}

template <size_t NUM_HASH_BLOCKS>
static void cryptonight_softaes(const void *input, size_t size, void *output, cryptonight_ctx *ctx) {
  cryptonight_multi_hash<0x80000, MEMORY, 0x1FFFF0, true, NUM_HASH_BLOCKS>(input, size, output, ctx);
}

template <size_t NUM_HASH_BLOCKS>
static void cryptonight_lite_aesni(const void *input, size_t size, void *output, cryptonight_ctx *ctx) {
#   if !defined(XMRIG_ARMv7)
  cryptonight_multi_hash<0x40000, MEMORY_LITE, 0xFFFF0, false, NUM_HASH_BLOCKS>(input, size, output, ctx);
#   endif
}

template <size_t NUM_HASH_BLOCKS>
static void cryptonight_lite_softaes(const void *input, size_t size, void *output, cryptonight_ctx *ctx) {
  cryptonight_multi_hash<0x40000, MEMORY_LITE, 0xFFFF0, true, NUM_HASH_BLOCKS>(input, size, output, ctx);
}

void (*cryptonight_hash_ctx[MAX_NUM_HASH_BLOCKS])(const void *input, size_t size, void *output, cryptonight_ctx *ctx);

template <size_t HASH_FACTOR>
void setCryptoNightHashMethods(Options::Algo algo, bool aesni)
{

    switch (algo) {
        case Options::ALGO_CRYPTONIGHT:
            if (aesni) {
                cryptonight_hash_ctx[HASH_FACTOR - 1] = cryptonight_aesni<HASH_FACTOR>;
            } else {
                cryptonight_hash_ctx[HASH_FACTOR - 1] = cryptonight_softaes<HASH_FACTOR>;
            }
            break;

        case Options::ALGO_CRYPTONIGHT_LITE:
            if (aesni) {
                cryptonight_hash_ctx[HASH_FACTOR - 1] = cryptonight_lite_aesni<HASH_FACTOR>;
            } else {
                cryptonight_hash_ctx[HASH_FACTOR - 1] = cryptonight_lite_softaes<HASH_FACTOR>;
            }
            break;
    }
    // next iteration
    setCryptoNightHashMethods<HASH_FACTOR-1>(algo, aesni);
}

template <>
void setCryptoNightHashMethods<0>(Options::Algo algo, bool aesni)
{
    // template recursion abort
};

bool CryptoNight::init(int algo, int variant)
{
    if (variant < 1 || variant  > 8)
    {
        return false;
    }

    setCryptoNightHashMethods<MAX_NUM_HASH_BLOCKS>(static_cast<Options::Algo>(algo), variant < 5);

    return selfTest(algo);
}

void CryptoNight::hash(size_t factor, const uint8_t* input, size_t size, uint8_t* output, cryptonight_ctx* ctx)
{
//  boost::chrono::high_resolution_clock::time_point timeBeforeWrite =
//      boost::chrono::high_resolution_clock::now();

  cryptonight_hash_ctx[factor-1](input, size, output, ctx);

//  int32_t timeHashDone =
//      boost::chrono::duration_cast<boost::chrono::microseconds>(
//          boost::chrono::high_resolution_clock::now() - timeBeforeWrite).count();
//  std::cout << "Single hash time = " << timeHashDone << "us" << std::endl;
}

bool CryptoNight::selfTest(int algo)
{
    if (cryptonight_hash_ctx[0] == nullptr || cryptonight_hash_ctx[2] == nullptr ||
        cryptonight_hash_ctx[2] == nullptr || cryptonight_hash_ctx[3] == nullptr) {
        return false;
    }

    char output[128];

    auto ctx = (struct cryptonight_ctx*) _mm_malloc(sizeof(struct cryptonight_ctx), 16);
    ctx->memory = (uint8_t *) _mm_malloc(MEMORY * 4, 16);

    cryptonight_hash_ctx[0](test_input, 76, output, ctx);
    bool resultSingle = memcmp(output, algo == Options::ALGO_CRYPTONIGHT_LITE ? test_output1 : test_output0, 32) == 0;

    cryptonight_hash_ctx[1](test_input, 76, output, ctx);
    bool resultDouble = memcmp(output, algo == Options::ALGO_CRYPTONIGHT_LITE ? test_output1 : test_output0, 64) == 0;

    cryptonight_hash_ctx[2](test_input, 76, output, ctx);
    bool resultTriple = memcmp(output, algo == Options::ALGO_CRYPTONIGHT_LITE ? test_output1 : test_output0, 96) == 0;

    cryptonight_hash_ctx[3](test_input, 76, output, ctx);
    bool resultQuad = memcmp(output, algo == Options::ALGO_CRYPTONIGHT_LITE ? test_output1 : test_output0, 128) == 0;

    _mm_free(ctx->memory);
    _mm_free(ctx);

    return resultSingle && resultDouble && resultTriple && resultQuad;
}
