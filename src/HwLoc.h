//
// Created by ses on 07.01.18.
//

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
    class HwLocObject : public TopologySharer {
    public:
        HwLocObject(const Topology &topo, hwloc_obj_t hwLocObject);
        hwloc_obj_t hwLocObject();
        const hwloc_obj_t hwLocObject() const;
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
    };

    // ------------------------------------------------------------------------
    class HwLoc : public TopologySharer {

    public:

    public:
        HwLoc();

        std::vector<Cache> getCaches(uint32_t level);

        std::vector<Core> getCores();

        size_t getNumberOfCores();

        void distriburtOverCpus(size_t numThreads);

    private:
        bool initialized() const;

    };

} // namespace hwloc

#endif //XMRIG_HWLOC_H
