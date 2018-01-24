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

#ifndef __CPU_H__
#define __CPU_H__

#include <cstdint>
#include <vector>
#include <memory>

#include "Options.h"

class ProcessingUnit
{
public:
    typedef std::shared_ptr<ProcessingUnit> Ptr;
protected:
    virtual ~ProcessingUnit() {}

public:
    /// Binds current thread to this ProcessingUnit
    virtual void bindThread() = 0;

    /// Allocates page-aligned memory bound to this ProcessingUnit's NUMA core
    virtual void* allocMemBind(size_t len) = 0;
};

class Cpu
{
public:
    enum Flags {
        X86_64 = 1,
        AES    = 2,
        BMI2   = 4
    };

    static void init();

    static void optimizeParameters(size_t& threadsCount, size_t& hashFactor, Options::Algo algo,
                                    size_t maxCpuUsage, bool safeMode);

    static std::vector<ProcessingUnit::Ptr> getDistributedProcessingUnits(size_t numThreads);

    static void setAffinity(int id, uint64_t mask);

    static bool hasAES();
    static bool isX64();
    static const char *brand();
    static size_t l2();
    static size_t l3();
    static size_t cores();
    static size_t sockets();
    static size_t threads();
    static size_t availableCache();
};


#endif /* __CPU_H__ */
