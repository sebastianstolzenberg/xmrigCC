/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2016-2017 XMRig       <support@xmrig.com>
 * Copyright 2018      Sebastian Stolzenberg <https://github.com/sebastianstolzenberg>
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

#include <algorithm>
#include <iostream>
#include <cpuid.h>

#include "HwLoc.h"
#include "Cpu.h"
#include "CpuImpl.h"

class HwlocProcessingUnit : public ProcessingUnit
{
public:
    HwlocProcessingUnit(const hwloc::CpuSet& hwlocPU) : m_hwlocCpuSet(hwlocPU)
    {
    }

    void bindThread()
    {
        m_hwlocCpuSet.bindThread();
    }

    void bindMemory()
    {
        m_hwlocCpuSet.enableMemoryBinding();
    }

    void* allocMemBind(size_t len)
    {
        return m_hwlocCpuSet.allocMemBind(len);
    }

private:
    hwloc::CpuSet m_hwlocCpuSet;
};

void CpuImpl::initCommon()
{
    hwloc::HwLoc topology;

    m_sockets = std::max(topology.getNumPackages(), static_cast<size_t>(1));
    m_totalCores = std::max(topology.getNumCores(), static_cast<size_t>(1));
    m_totalThreads = std::max(topology.getNumProcessingUnits(), static_cast<size_t>(1));

    // L3 caches
    std::vector<hwloc::Cache> caches = topology.getCaches(3);
    m_l3_cache = 0;
    for (auto cache : caches) {
        m_l3_cache += cache.size();
    }
    m_l3_cache /= 1024;

    // L2 caches
    caches = topology.getCaches(2);
    m_l2_cache = 0;
    for (auto cache : caches) {
        m_l2_cache += cache.size();
    }
    m_l2_cache /= 1024;

    std::cout << "Cpu_hwloc"
              << ", m_sockets , " << m_sockets
              << ", m_totalCores , " << m_totalCores
              << ", m_totalThreads , " << m_totalThreads
              << ", m_l3_cache , " << m_l3_cache
              << ", m_l2_cache , " << m_l2_cache << std::endl;

    //TODO Workaround for AMD CPUs https://github.com/anrieff/libcpuid/issues/97
//    if (data.vendor == VENDOR_AMD && data.ext_family >= 0x15 && data.ext_family < 0x17) {
//        m_l2_exclusive = true;
//    }

    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
    ;


#   if defined(__x86_64__) || defined(_M_AMD64)
  m_flags |= Cpu::X86_64;
#   endif

    __get_cpuid(1, &eax, &ebx, &ecx, &edx);
    if (ecx & bit_AES) {
      m_flags |= Cpu::AES;
    }

  __get_cpuid(7, &eax, &ebx, &ecx, &edx);
    if (ebx & 0x00000080)
    {
      m_flags |= Cpu::BMI2;
    }
}

std::vector<ProcessingUnit::Ptr> CpuImpl::getDistributedProcessingUnits(size_t numThreads)
{
    std::vector<ProcessingUnit::Ptr> pus;
    hwloc::HwLoc topology;
    std::vector<hwloc::CpuSet> cpuSets = topology.getDistributedCpuSets(numThreads);
    for (auto cpuSet : cpuSets) {
        pus.push_back(std::make_shared<HwlocProcessingUnit>(cpuSet));
    }
    return pus;
}
