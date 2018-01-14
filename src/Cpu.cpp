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


#include <cmath>
#include <cstring>
#include <algorithm>
#include <memory>

#include <libcpuid.h>
#include <iostream>

#include "Cpu.h"
#include "HwLoc.h"

class CpuImpl : public Cpu
{
public:
    CpuImpl();
    void init();

    void optimizeParameters(size_t& threadsCount, size_t& hashFactor, Options::Algo algo,
                            int maxCpuUsage, bool safeMode);
    void setAffinity(int id, uint64_t mask);

    bool hasAES()       { return (m_flags & AES) != 0; }
    bool isX64()        { return (m_flags & X86_64) != 0; }
    const char *brand() { return m_brand; }
    int cores()         { return m_totalCores; }
    int l2()            { return m_l2_cache; }
    int l3()            { return m_l3_cache; }
    int sockets()       { return m_sockets; }
    int threads()       { return m_totalThreads; }
    size_t availableCache();

private:
    void initCommon();

    bool m_l2_exclusive;
    char m_brand[64];
    int m_flags;
    int m_l2_cache;
    int m_l3_cache;
    size_t m_sockets;
    size_t m_totalCores;
    size_t m_totalThreads;

    std::vector<hwloc_cpuset_t> m_theadDistribution;
};

void testHwLoc() {
    hwloc::HwLoc hwloc;

    std::cout << std::endl << "---------------------------------------------------" << std::endl;
    std::cout             << "HWLOC: Analyzing Caches" << std::endl;
    auto caches = hwloc.getCaches(3);
    std::cout             << "HWLOC: found " << caches.size()
                          << " L3 Cache(s)" << std::endl;
    if (caches.empty()) {
        caches = hwloc.getCaches(2);
        std::cout         << "HWLOC: found " << caches.size() << " L2 Cache(s)" << std::endl;
    }
    for (auto cache : caches) {
        std::cout         << "HWLOC: |-" << cache.toString() << std::endl;
        auto cores = cache.cores();
        std::cout         << "HWLOC: | |has " << cores.size()
                          << " core(s)" << std::endl;
        for (auto core : cache.cores()) {
            std::cout     << "HWLOC: | |-" << core.toString() << std::endl;
            auto processingUnits = core.processingUnits();
            std::cout     << "HWLOC: | | |has " << processingUnits.size()
                          << " processing unit(s)" << std::endl;
            for (auto pu : processingUnits) {
                std::cout << "HWLOC: | | | -" << pu.toString() << std::endl;
            }
        }
    }
    std::cout << "---------------------------------------------------" << std::endl << std::endl;

    std::cout << "---------------------------------------------------" << std::endl;
    std::cout             << "HWLOC: Analyzing Cores" << std::endl;
    auto cores = hwloc.getCores();
    std::cout             << "HWLOC: found " << cores.size()
                          << " Core(s)" << std::endl;
    for (auto core : cores) {
        std::cout         << "HWLOC: |-" << core.toString() << std::endl;
        auto processingUnits = core.processingUnits();
        std::cout         << "HWLOC: | |has " << processingUnits.size()
                          << " processingUnit(s)" << std::endl;
        for (auto pu : core.processingUnits()) {
            std::cout     << "HWLOC: | |-" << pu.toString() << std::endl;
        }
    }
    std::cout << "---------------------------------------------------" << std::endl << std::endl;

    std::cout << "---------------------------------------------------" << std::endl;
    std::cout             << "HWLOC: Analyzing Processing Units" << std::endl;
    auto processingUnits = hwloc.getProcessingUnits();
    std::cout             << "HWLOC: found " << processingUnits.size()
                          << " Processing Unit(s)" << std::endl;
    for (auto pu : processingUnits) {
        std::cout         << "HWLOC: |-" << pu.toString() << std::endl;
    }
    std::cout << "---------------------------------------------------" << std::endl << std::endl;

    //hwloc.distriburtOverCpus(hwloc.getNumberOfCores());
}

Cpu& Cpu::instance() {
    static CpuImpl cpu;
    return cpu;
}

CpuImpl::CpuImpl()
    : m_l2_exclusive(false)
    , m_brand{ 0 }
    , m_flags(0)
    , m_l2_cache(0)
    , m_l3_cache(0)
    , m_sockets(1)
    , m_totalCores(0)
    , m_totalThreads(0) {
    init();
}

void CpuImpl::init()
{
#   ifdef XMRIG_NO_LIBCPUID
    m_totalThreads = sysconf(_SC_NPROCESSORS_CONF);
#   endif

    initCommon();
}

void CpuImpl::optimizeParameters(size_t& threadsCount, size_t& hashFactor,
                                 Options::Algo algo, int maxCpuUsage, bool safeMode)
{
    // limits hashfactor to maximum possible value defined by compiler flag
    hashFactor = std::min(hashFactor, static_cast<size_t>(MAX_NUM_HASH_BLOCKS));

    if (!safeMode && threadsCount > 0 && hashFactor > 0)
    {
      // all parameters have been set manually, no optimization necessary
      return;
    }

    size_t cache = availableCache();
    size_t algoBlockSize;
    switch (algo) {
        case Options::ALGO_CRYPTONIGHT_LITE:
            algoBlockSize = 1024;
            break;
        case Options::ALGO_CRYPTONIGHT:
        default:
            algoBlockSize = 2048;
            break;
    }

    size_t maximumReasonableFactor = std::max(cache / algoBlockSize, 1ul);
    size_t maximumReasonableThreadCount = std::min(maximumReasonableFactor, m_totalThreads);
    size_t maximumReasonableHashFactor = std::min(maximumReasonableFactor, static_cast<size_t>(MAX_NUM_HASH_BLOCKS));

    if (safeMode) {
        if (threadsCount > maximumReasonableThreadCount) {
            threadsCount = maximumReasonableThreadCount;
        }
        if (hashFactor > maximumReasonableFactor / threadsCount) {
            hashFactor = std::min(maximumReasonableFactor / threadsCount, maximumReasonableHashFactor);
        }
    }

    if (threadsCount == 0) {
        if (hashFactor == 0) {
            threadsCount = maximumReasonableThreadCount;
        }
        else {
            threadsCount = std::min(maximumReasonableThreadCount,
                                    maximumReasonableFactor / hashFactor);
        }
    }
    if (hashFactor == 0) {
        hashFactor = std::min(maximumReasonableHashFactor, maximumReasonableFactor / threadsCount);
    }

    threadsCount = std::max(threadsCount, 1ul);
    hashFactor   = std::max(hashFactor, 1ul);
}

size_t CpuImpl::availableCache()
{
    size_t cache = 0;
    if (m_l3_cache) {
        cache = m_l2_exclusive ? (m_l2_cache + m_l3_cache) : m_l3_cache;
    }
    else {
        cache = m_l2_cache;
    }
    return cache;
}

void CpuImpl::initCommon()
{
    // testHwLoc();

    struct cpu_raw_data_t raw = { 0 };
    struct cpu_id_t data = { 0 };

    cpuid_get_raw_data(&raw);
    cpu_identify(&raw, &data);

    strncpy(m_brand, data.brand_str, sizeof(m_brand) - 1);

    m_totalThreads = data.total_logical_cpus;
    m_sockets      = m_totalThreads / data.num_logical_cpus;

    if (m_sockets == 0) {
        m_sockets = 1;
    }

    m_totalCores = data.num_cores * m_sockets;
    m_l3_cache = data.l3_cache > 0 ? data.l3_cache * m_sockets : 0;

    // Workaround for AMD CPUs https://github.com/anrieff/libcpuid/issues/97
    if (data.vendor == VENDOR_AMD && data.ext_family >= 0x15 && data.ext_family < 0x17) {
        m_l2_cache = data.l2_cache * (m_totalCores / 2) * m_sockets;
        m_l2_exclusive = true;
    }
    else {
        m_l2_cache = data.l2_cache > 0 ? data.l2_cache * m_totalCores * m_sockets : 0;
    }

#   if defined(__x86_64__) || defined(_M_AMD64)
    m_flags |= X86_64;
#   endif

    if (data.flags[CPU_FEATURE_AES]) {
        m_flags |= AES;
    }

    if (data.flags[CPU_FEATURE_BMI2]) {
        m_flags |= BMI2;
    }
}

void CpuImpl::setAffinity(int id, uint64_t mask)
{
    cpu_set_t set;
    CPU_ZERO(&set);

    for (size_t i = 0; i < m_totalThreads; i++) {
        if (mask & (1UL << i)) {
            CPU_SET(i, &set);
        }
    }

    if (id == -1) {
#       ifndef __FreeBSD__
        sched_setaffinity(0, sizeof(&set), &set);
#       endif
    } else {
#       ifndef __ANDROID__
        pthread_setaffinity_np(pthread_self(), sizeof(&set), &set);
#       else
        sched_setaffinity(gettid(), sizeof(&set), &set);
#       endif
    }
}
