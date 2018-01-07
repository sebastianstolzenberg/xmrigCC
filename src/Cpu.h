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


class Cpu
{
public:
    enum Flags {
        X86_64 = 1,
        AES    = 2,
        BMI2   = 4
    };

    static Cpu& instance();

    virtual size_t optimalThreadsCount(int algo, int hashFactor, int maxCpuUsage) = 0;
    virtual size_t optimalHashFactor(int algo, int threadsCount) = 0;
    virtual void setAffinity(int id, uint64_t mask) = 0;

    virtual bool hasAES() = 0;
    virtual bool isX64() = 0;
    virtual const char *brand() = 0;
    virtual int cores() = 0;
    virtual int l2() = 0;
    virtual int l3() = 0;
    virtual int sockets() = 0;
    virtual int threads() = 0;

protected:
    virtual ~Cpu() {}
};


#endif /* __CPU_H__ */
