#include <unity.h>
#include <libcpuid.h>
#include <iostream>

#include "HwLoc.h"

void test_hwloc(void)
{
    hwloc::HwLoc hwloc;

    std::cout << "\n\nROOT\n" << hwloc.root().toString();

    for (auto package : hwloc.getPackages()) {
      std::cout << "\n\nPACKAGE\n" << package.toString();
    }

    for (auto cache : hwloc.getCaches(3)) {
      std::cout << "\n\nCACHE\n" << cache.toString();
    }

    for (auto core : hwloc.getCores()) {
      std::cout << "\n\nCORE\n" << core.toString();
    }

    for (auto pu : hwloc.getProcessingUnits()) {
      std::cout << "\n\nPU\n" << pu.toString();
    }


//    TEST_ASSERT(Expected(10,10) == testOptimize(10, 10, Options::ALGO_CRYPTONIGHT, false));
//    TEST_ASSERT(Expected(10,10) == testOptimize(10, 10, Options::ALGO_CRYPTONIGHT_LITE, false));
//    TEST_ASSERT(Expected(1,1) == testOptimize(10, 10, Options::ALGO_CRYPTONIGHT, true));
//    TEST_ASSERT(Expected(1,1) == testOptimize(10, 10, Options::ALGO_CRYPTONIGHT_LITE, true));
}
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_hwloc);

    return UNITY_END();
}
