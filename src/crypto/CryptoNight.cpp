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


#include "crypto/CryptoNight.h"

#if defined(XMRIG_ARM)
#   include "crypto/CryptoNight_arm.h"
#else
#   include "crypto/CryptoNight_x86.h"
#endif

#include "crypto/CryptoNight_test.h"
#include "Options.h"

void (*cryptonight_hash_ctx_s)(const void *input, size_t size, void *output, cryptonight_ctx *ctx)  = nullptr;
void (*cryptonight_hash_ctx_d)(const void *input, size_t size, void *output, cryptonight_ctx *ctx) = nullptr;
void (*cryptonight_hash_ctx_t)(const void *input, size_t size, void *output, cryptonight_ctx *ctx) = nullptr;
void (*cryptonight_hash_ctx_q)(const void *input, size_t size, void *output, cryptonight_ctx *ctx) = nullptr;


static void cryptonight_av1_aesni(const void *input, size_t size, void *output, struct cryptonight_ctx *ctx) {
#   if !defined(XMRIG_ARMv7)
    cryptonight_multi_hash<0x80000, MEMORY, 0x1FFFF0, false, 1>(input, size, output, ctx);
#   endif
}


static void cryptonight_av2_aesni_double(const void *input, size_t size, void *output, cryptonight_ctx *ctx) {
#   if !defined(XMRIG_ARMv7)
    cryptonight_multi_hash<0x80000, MEMORY, 0x1FFFF0, false, 2>(input, size, output, ctx);
#   endif
}

static void cryptonight_av3_aesni_triple(const void *input, size_t size, void *output, cryptonight_ctx *ctx) {
#   if !defined(XMRIG_ARMv7)
    cryptonight_multi_hash<0x80000, MEMORY, 0x1FFFF0, false, 3>(input, size, output, ctx);
#   endif
}

static void cryptonight_av4_aesni_quad(const void *input, size_t size, void *output, cryptonight_ctx *ctx) {
#   if !defined(XMRIG_ARMv7)
    cryptonight_multi_hash<0x80000, MEMORY, 0x1FFFF0, false, 4>(input, size, output, ctx);
#   endif
}

static void cryptonight_av5_softaes(const void *input, size_t size, void *output, cryptonight_ctx *ctx) {
    cryptonight_multi_hash<0x80000, MEMORY, 0x1FFFF0, true, 1>(input, size, output, ctx);
}


static void cryptonight_av6_softaes_double(const void *input, size_t size, void *output, cryptonight_ctx *ctx) {
    cryptonight_multi_hash<0x80000, MEMORY, 0x1FFFF0, true, 2>(input, size, output, ctx);
}


static void cryptonight_av7_softaes_triple(const void *input, size_t size, void *output, cryptonight_ctx *ctx) {
    cryptonight_multi_hash<0x80000, MEMORY, 0x1FFFF0, true, 3>(input, size, output, ctx);
}


static void cryptonight_av8_softaes_quad(const void *input, size_t size, void *output, cryptonight_ctx *ctx) {
    cryptonight_multi_hash<0x80000, MEMORY, 0x1FFFF0, true, 4>(input, size, output, ctx);
}

static void cryptonight_lite_av1_aesni(const void *input, size_t size, void *output, cryptonight_ctx *ctx) {
#   if !defined(XMRIG_ARMv7)
    cryptonight_multi_hash<0x40000, MEMORY_LITE, 0xFFFF0, false, 1>(input, size, output, ctx);
#endif
}

static void cryptonight_lite_av2_aesni_double(const void *input, size_t size, void *output, cryptonight_ctx *ctx) {
#   if !defined(XMRIG_ARMv7)
    cryptonight_multi_hash<0x40000, MEMORY_LITE, 0xFFFF0, false, 2>(input, size, output, ctx);
#   endif
}

static void cryptonight_lite_av3_aesni_triple(const void *input, size_t size, void *output, cryptonight_ctx *ctx) {
#   if !defined(XMRIG_ARMv7)
    cryptonight_multi_hash<0x40000, MEMORY_LITE, 0xFFFF0, false, 3>(input, size, output, ctx);
#   endif
}

static void cryptonight_lite_av4_aesni_quad(const void *input, size_t size, void *output, cryptonight_ctx *ctx) {
#   if !defined(XMRIG_ARMv7)
    cryptonight_multi_hash<0x40000, MEMORY_LITE, 0xFFFF0, false, 4>(input, size, output, ctx);
#   endif
}


static void cryptonight_lite_av5_softaes(const void *input, size_t size, void *output, cryptonight_ctx *ctx) {
    cryptonight_multi_hash<0x40000, MEMORY_LITE, 0xFFFF0, true, 1>(input, size, output, ctx);
}

static void cryptonight_lite_av6_softaes_double(const void *input, size_t size, void *output, cryptonight_ctx *ctx) {
    cryptonight_multi_hash<0x40000, MEMORY_LITE, 0xFFFF0, true, 2>(input, size, output, ctx);
}

static void cryptonight_lite_av7_softaes_triple(const void *input, size_t size, void *output, cryptonight_ctx *ctx) {
    cryptonight_multi_hash<0x40000, MEMORY_LITE, 0xFFFF0, true, 3>(input, size, output, ctx);
}

static void cryptonight_lite_av8_softaes_quad(const void *input, size_t size, void *output, cryptonight_ctx *ctx) {
    cryptonight_multi_hash<0x40000, MEMORY_LITE, 0xFFFF0, true, 4>(input, size, output, ctx);
}


void (*cryptonight_variations[16])(const void *input, size_t size, void *output, cryptonight_ctx *ctx) = {
            cryptonight_av1_aesni,
            cryptonight_av2_aesni_double,
            cryptonight_av3_aesni_triple,
            cryptonight_av4_aesni_quad,
            cryptonight_av5_softaes,
            cryptonight_av6_softaes_double,
            cryptonight_av7_softaes_triple,
            cryptonight_av8_softaes_quad,
            cryptonight_lite_av1_aesni,
            cryptonight_lite_av2_aesni_double,
            cryptonight_lite_av3_aesni_triple,
            cryptonight_lite_av4_aesni_quad,
            cryptonight_lite_av5_softaes,
            cryptonight_lite_av6_softaes_double,
            cryptonight_lite_av7_softaes_triple,
            cryptonight_lite_av8_softaes_quad
};


bool CryptoNight::init(int algo, int variant)
{
    if (variant < 1 || variant  > 8)
    {
        return false;
    }

    int index = 0;

    if (algo == Options::ALGO_CRYPTONIGHT_LITE) {
        index += 8;
    }

    if (variant >= 5 && variant <= 8)
    {
        index += 4;
    }

    cryptonight_hash_ctx_s = cryptonight_variations[index];
    cryptonight_hash_ctx_d = cryptonight_variations[index+1];
    cryptonight_hash_ctx_t = cryptonight_variations[index+2];
    cryptonight_hash_ctx_q = cryptonight_variations[index+3];

    return selfTest(algo);
}

bool CryptoNight::selfTest(int algo)
{
    if (cryptonight_hash_ctx_s == nullptr || cryptonight_hash_ctx_d == nullptr ||
        cryptonight_hash_ctx_t == nullptr || cryptonight_hash_ctx_q == nullptr) {
        return false;
    }

    char output[128];

    auto ctx = (struct cryptonight_ctx*) _mm_malloc(sizeof(struct cryptonight_ctx), 16);
    ctx->memory = (uint8_t *) _mm_malloc(MEMORY * 4, 16);

    cryptonight_hash_ctx_s(test_input, 76, output, ctx);
    bool resultSingle = memcmp(output, algo == Options::ALGO_CRYPTONIGHT_LITE ? test_output1 : test_output0, 32) == 0;

    cryptonight_hash_ctx_d(test_input, 76, output, ctx);
    bool resultDouble = memcmp(output, algo == Options::ALGO_CRYPTONIGHT_LITE ? test_output1 : test_output0, 64) == 0;

    cryptonight_hash_ctx_t(test_input, 76, output, ctx);
    bool resultTriple = memcmp(output, algo == Options::ALGO_CRYPTONIGHT_LITE ? test_output1 : test_output0, 96) == 0;

    cryptonight_hash_ctx_q(test_input, 76, output, ctx);
    bool resultQuad = memcmp(output, algo == Options::ALGO_CRYPTONIGHT_LITE ? test_output1 : test_output0, 128) == 0;

    _mm_free(ctx->memory);
    _mm_free(ctx);

    return resultSingle && resultDouble && resultTriple && resultQuad;
}
