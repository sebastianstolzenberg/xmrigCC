#include <unity.h>
#include <libcpuid.h>

#include "Options.h"
#include "Cpu.h"

struct cpu_id_t mockCpuId;

int cpuid_get_raw_data(struct cpu_raw_data_t* data)
{
    return 0;
}

int cpu_identify(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
    memcpy(data, &mockCpuId, sizeof(struct cpu_id_t));
    return 0;
}

void test_cpu_optimizeparameters(void)
{
    strcpy(mockCpuId.brand_str, "CPU Test Brand");
    mockCpuId.vendor = VENDOR_INTEL;

    mockCpuId.total_logical_cpus = 1;
    mockCpuId.num_logical_cpus = 1;
    mockCpuId.num_cores = 1;
    mockCpuId.l3_cache = 2048;
    mockCpuId.l2_cache = 128;


    size_t availableCache = Cpu::instance().availableCache();
    size_t maxFactorCryptonight = availableCache / 2024;
    size_t maxFactorCryptonightLite = availableCache / 1024;

    TEST_ASSERT(availableCache > 0);

    size_t numThreads = 1;
    size_t hashFactor = 1;
    Cpu::instance().optimizeParameters(numThreads, hashFactor, Options::ALGO_CRYPTONIGHT, 100, false);
    TEST_ASSERT_EQUAL_UINT32(1, numThreads);
    TEST_ASSERT_EQUAL_UINT32(1, hashFactor);

    numThreads = maxFactorCryptonight;
    hashFactor = 1;
    Cpu::instance().optimizeParameters(numThreads, hashFactor, Options::ALGO_CRYPTONIGHT, 100, false);
    TEST_ASSERT_EQUAL_UINT32(maxFactorCryptonight, numThreads);
    TEST_ASSERT_EQUAL_UINT32(1, hashFactor);


//    free_ctx(ctx);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_cpu_optimizeparameters);

    return UNITY_END();
}
