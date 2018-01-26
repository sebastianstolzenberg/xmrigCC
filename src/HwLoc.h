/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2016-2017 XMRig       <support@xmrig.com>
 * Copyright 2018-     Sebastian Stolzenberg <https://github.com/sebastianstolzenberg>
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

#ifndef XMRIG_HWLOC_H
#define XMRIG_HWLOC_H

#include <memory>
#include <vector>

#include <hwloc.h>

namespace hwloc {
    typedef std::shared_ptr<hwloc_topology> Topology;

    // ------------------------------------------------------------------------
    class TopologySharer {
    public:
        TopologySharer();
        TopologySharer(const Topology &topo);
    protected:
        void setTopology(hwloc_topology_t topo);
        bool isTopologySet() const;
        Topology sharedTopology();
        hwloc_topology_t topology();
    private:
        Topology m_topology; // kept so that m_hwLocObject remains valid
    };

    // ------------------------------------------------------------------------
    class CpuSet : public TopologySharer
    {
    public:
        CpuSet(const Topology& topo, hwloc_cpuset_t cpuSet);

        void bindThread();
        void* allocMemBind(size_t len);

    private:
        hwloc_cpuset_t m_hwLocCpuSet;
    };

    // ------------------------------------------------------------------------
    class HwLocObject : public TopologySharer
    {
    public:
        HwLocObject(const Topology &topo, hwloc_obj_t hwLocObject);
        std::string toString() const;
        hwloc_obj_t hwLocObject();
        const hwloc_obj_t hwLocObject() const;

        CpuSet cpuSet();

    protected:
        size_t getNumContained(hwloc_obj_type_t type);

        template <class C>
        std::vector<C> getContained(hwloc_obj_type_t type);

    private:
        hwloc_obj_t m_hwLocObject;
    };

    // ------------------------------------------------------------------------
    class ProcessingUnit : public HwLocObject {
    public:
        ProcessingUnit(const Topology &topo, hwloc_obj_t hwLocObject);
        std::string toString() const;

    };

    // ------------------------------------------------------------------------
    class Core : public HwLocObject {
    public:
        Core(const Topology &topo, hwloc_obj_t hwLocObject);
        std::string toString() const;
        // virtual processing units
        std::vector<ProcessingUnit> processingUnits();
    };

    // ------------------------------------------------------------------------
    class Cache : public HwLocObject {
    public:
        Cache(const Topology &topo, hwloc_obj_t hwLocObject);
        std::string toString() const;
        // cache size
        size_t size() const;
        // physical cores
        std::vector<Core> cores();
        // virtual processing units
        std::vector<ProcessingUnit> processingUnits();
    };

    // ------------------------------------------------------------------------
    class Package : public HwLocObject {
    public:
      Package(const Topology &topo, hwloc_obj_t hwLocObject);
    };

    // ------------------------------------------------------------------------
    class Root : public HwLocObject {
    public:
        Root(const Topology &topo, hwloc_obj_t hwLocObject);
        std::string toString() const;
        // packages
        size_t numPackages();
        std::vector<Package> packages();
        // caches
        size_t numCaches(uint32_t level);
        std::vector<Cache> caches(uint32_t level);
        // physical cores
        size_t numCores();
        std::vector<Core> cores();
        // virtual processing units
        size_t numProcessingUnits();
        std::vector<ProcessingUnit> processingUnits();
    };

    // ------------------------------------------------------------------------
    class HwLoc : public TopologySharer {

    public:

    public:
        HwLoc();

        Root root();
        size_t getNumPackages();
        std::vector<Package> getPackages();
        size_t getNumCaches(uint32_t level);
        std::vector<Cache> getCaches(uint32_t level);
        size_t getNumCores();
        std::vector<Core> getCores();
        size_t getNumProcessingUnits();
        std::vector<ProcessingUnit> getProcessingUnits();
        std::vector<CpuSet> getDistributedCpuSets(size_t num);

        void distributeOverCpus(size_t numThreads);

    private:
        bool initialized() const;

    };

} // namespace hwloc

#endif //XMRIG_HWLOC_H
