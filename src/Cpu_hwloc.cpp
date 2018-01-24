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


#include "HwLoc.h"
#include "CpuImpl.h"

void CpuImpl::initCommon()
{
    hwloc::HwLoc topology;

    m_sockets = std::max(topology.getNumSockets(), 1);
    m_totalCores = std::max(topology.getNumCores(), 1);
    m_totalThreads = std::max(topology.getNumProcessingUnits(), 1);

    // L3 caches
    std::vector<hwloc::Cache> caches = topology.getCaches(3);
    m_l3_cache = 0;
    for (auto cache : caches) {
        m_l3_cache += cache.size();
    }

    // L2 caches
    caches = topology.getCaches(2);
    m_l2_cache = 0;
    for (auto cache : caches) {
        m_l2_cache += cache.size();
    }


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

    //TODO Workaround for AMD CPUs https://github.com/anrieff/libcpuid/issues/97
//    if (data.vendor == VENDOR_AMD && data.ext_family >= 0x15 && data.ext_family < 0x17) {
//        m_l2_exclusive = true;
//    }

#   if defined(__x86_64__) || defined(_M_AMD64)
    m_flags |= Cpu::X86_64;
#   endif

    if (data.flags[CPU_FEATURE_AES]) {
        m_flags |= Cpu::AES;
    }

    if (data.flags[CPU_FEATURE_BMI2]) {
        m_flags |= Cpu::BMI2;
    }
}
