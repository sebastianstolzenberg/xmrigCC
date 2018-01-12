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

void setMockedCpu(size_t numProcessors, size_t numCores, size_t numPusPerCore, size_t l3Cache)
{
  strcpy(mockCpuId.brand_str, "CPU Test Brand");
  mockCpuId.vendor = VENDOR_INTEL;

  mockCpuId.num_cores = numCores;
  mockCpuId.num_logical_cpus = numCores * numPusPerCore;
  mockCpuId.total_logical_cpus = mockCpuId.num_logical_cpus * numProcessors;
  mockCpuId.l3_cache = 1024;
  mockCpuId.l2_cache = 128;
}

void test_cpu_optimizeparameters_cores1_cache1mb(void)
{
    const size_t NUM_PROCESSORS = 1;
    const size_t NUM_CORES = 1;
    const size_t NUM_PUS_PER_CORE = 1;
    const size_t L3_CACHE = 1024;
    setMockedCpu(NUM_PROCESSORS, NUM_CORES, NUM_PUS_PER_CORE, L3_CACHE);

    TEST_ASSERT_EQUAL_UINT32(Cpu::instance().availableCache(), L3_CACHE);

    size_t numThreads = 0; size_t hashFactor = 0;
    Cpu::instance().optimizeParameters(numThreads, hashFactor, Options::ALGO_CRYPTONIGHT, 100, false);
    TEST_ASSERT_EQUAL_UINT32(1, numThreads);
    TEST_ASSERT_EQUAL_UINT32(1, hashFactor);
    Cpu::instance().optimizeParameters(numThreads, hashFactor, Options::ALGO_CRYPTONIGHT_LITE, 100, false);
    TEST_ASSERT_EQUAL_UINT32(1, numThreads);
    TEST_ASSERT_EQUAL_UINT32(1, hashFactor);

    numThreads = 1; hashFactor = 1;
    Cpu::instance().optimizeParameters(numThreads, hashFactor, Options::ALGO_CRYPTONIGHT, 100, false);
    TEST_ASSERT_EQUAL_UINT32(1, numThreads);
    TEST_ASSERT_EQUAL_UINT32(1, hashFactor);
    Cpu::instance().optimizeParameters(numThreads, hashFactor, Options::ALGO_CRYPTONIGHT_LITE, 100, false);
    TEST_ASSERT_EQUAL_UINT32(1, numThreads);
    TEST_ASSERT_EQUAL_UINT32(1, hashFactor);

    numThreads = 10; hashFactor = 1;
    Cpu::instance().optimizeParameters(numThreads, hashFactor, Options::ALGO_CRYPTONIGHT, 100, false);
    TEST_ASSERT_EQUAL_UINT32(10, numThreads);
    TEST_ASSERT_EQUAL_UINT32(1, hashFactor);
    Cpu::instance().optimizeParameters(numThreads, hashFactor, Options::ALGO_CRYPTONIGHT_LITE, 100, false);
    TEST_ASSERT_EQUAL_UINT32(10, numThreads);
    TEST_ASSERT_EQUAL_UINT32(1, hashFactor);
    Cpu::instance().optimizeParameters(numThreads, hashFactor, Options::ALGO_CRYPTONIGHT, 100, true);
    TEST_ASSERT_EQUAL_UINT32(1, numThreads);
    TEST_ASSERT_EQUAL_UINT32(1, hashFactor);
    numThreads = 10; hashFactor = 1;
    Cpu::instance().optimizeParameters(numThreads, hashFactor, Options::ALGO_CRYPTONIGHT_LITE, 100, true);
    TEST_ASSERT_EQUAL_UINT32(1, numThreads);
    TEST_ASSERT_EQUAL_UINT32(1, hashFactor);

    numThreads = 1; hashFactor = 10;
    Cpu::instance().optimizeParameters(numThreads, hashFactor, Options::ALGO_CRYPTONIGHT, 100, false);
    TEST_ASSERT_EQUAL_UINT32(1, numThreads);
    TEST_ASSERT_EQUAL_UINT32(10, hashFactor);
    Cpu::instance().optimizeParameters(numThreads, hashFactor, Options::ALGO_CRYPTONIGHT_LITE, 100, false);
    TEST_ASSERT_EQUAL_UINT32(1, numThreads);
    TEST_ASSERT_EQUAL_UINT32(10, hashFactor);
    Cpu::instance().optimizeParameters(numThreads, hashFactor, Options::ALGO_CRYPTONIGHT, 100, true);
    TEST_ASSERT_EQUAL_UINT32(1, numThreads);
    TEST_ASSERT_EQUAL_UINT32(1, hashFactor);
    numThreads = 1; hashFactor = 10;
    Cpu::instance().optimizeParameters(numThreads, hashFactor, Options::ALGO_CRYPTONIGHT_LITE, 100, true);
    TEST_ASSERT_EQUAL_UINT32(1, numThreads);
    TEST_ASSERT_EQUAL_UINT32(1, hashFactor);

    numThreads = 10; hashFactor = 10;
    Cpu::instance().optimizeParameters(numThreads, hashFactor, Options::ALGO_CRYPTONIGHT, 100, false);
    TEST_ASSERT_EQUAL_UINT32(10, numThreads);
    TEST_ASSERT_EQUAL_UINT32(10, hashFactor);
    Cpu::instance().optimizeParameters(numThreads, hashFactor, Options::ALGO_CRYPTONIGHT_LITE, 100, false);
    TEST_ASSERT_EQUAL_UINT32(10, numThreads);
    TEST_ASSERT_EQUAL_UINT32(10, hashFactor);
    Cpu::instance().optimizeParameters(numThreads, hashFactor, Options::ALGO_CRYPTONIGHT, 100, true);
    TEST_ASSERT_EQUAL_UINT32(1, numThreads);
    TEST_ASSERT_EQUAL_UINT32(1, hashFactor);
    numThreads = 10; hashFactor = 10;
    Cpu::instance().optimizeParameters(numThreads, hashFactor, Options::ALGO_CRYPTONIGHT_LITE, 100, true);
    TEST_ASSERT_EQUAL_UINT32(1, numThreads);
    TEST_ASSERT_EQUAL_UINT32(1, hashFactor);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_cpu_optimizeparameters_cores1_cache1mb);

    return UNITY_END();
}
