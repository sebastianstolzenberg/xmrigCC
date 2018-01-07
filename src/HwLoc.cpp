//
// Created by ses on 07.01.18.
//


#include <iostream>
#include <sstream>
#include "HwLoc.h"

namespace hwloc {

    // ------------------------------------------------------------------------
    TopologySharer::TopologySharer() {
    }

    TopologySharer::TopologySharer(const Topology &topo) :
        m_topology(topo) {
    }

    void TopologySharer::setTopology(hwloc_topology_t topo) {
        m_topology = Topology(topo, hwloc_topology_destroy);
    }

    bool TopologySharer::isTopologySet() const {
        return static_cast<bool>(m_topology);
    }

    Topology TopologySharer::sharedTopology() {
        return m_topology;
    }

    hwloc_topology_t TopologySharer::topology() {
        return m_topology.get();
    }


    // ------------------------------------------------------------------------
    HwLocObject::HwLocObject(const Topology &topo, hwloc_obj_t hwLocObject) :
            TopologySharer(topo),
            m_hwLocObject(hwLocObject) {
        if (hwLocObject == nullptr) {
          std::cout << "ERR: hwLocObject is null";
        }
    }

    hwloc_obj_t HwLocObject::hwLocObject() {
        return m_hwLocObject;
    };


    const hwloc_obj_t HwLocObject::hwLocObject() const {
        return m_hwLocObject;
    }


    // ------------------------------------------------------------------------
    ProcessingUnit::ProcessingUnit(const Topology &topo, hwloc_obj_t hwLocObject) :
            HwLocObject(topo, hwLocObject) {
    }

    std::string ProcessingUnit::toString() const {
        std::ostringstream out;
        out << "ProcessingUnit"
            << "_l" << hwLocObject()->logical_index
            << "_os" << hwLocObject()->os_index;
        return out.str();
    }

    // ------------------------------------------------------------------------
    Core::Core(const Topology &topo, hwloc_obj_t hwLocObject) :
            HwLocObject(topo, hwLocObject) {
    }

    std::string Core::toString() const {
        std::ostringstream out;
        out << "Core"
            << "_l" << hwLocObject()->logical_index
            << "_os" << hwLocObject()->os_index;
        return out.str();
    }

    std::vector<ProcessingUnit> Core::processingUnits() {
        std::vector<ProcessingUnit> processingUnits;

        int numProcessingUnits = hwloc_get_nbobjs_inside_cpuset_by_type(topology(), hwLocObject()->cpuset, HWLOC_OBJ_PU);
        if (numProcessingUnits > 0) {
            processingUnits.reserve(numProcessingUnits);
            hwloc_obj_t current = nullptr;
            for (int i=0; i<numProcessingUnits; ++i)
            {
                current = hwloc_get_next_obj_inside_cpuset_by_type(topology(), hwLocObject()->cpuset, HWLOC_OBJ_PU, current);
                processingUnits.push_back(ProcessingUnit(sharedTopology(), current));
            }
        }
        else {
            // no cores found
        }
        return processingUnits;
    }

    // ------------------------------------------------------------------------
    Cache::Cache(const Topology &topo, hwloc_obj_t hwLocObject) :
            HwLocObject(topo, hwLocObject) {
    }

    std::string Cache::toString() const {
        std::ostringstream out;
        out << "Cache " << hwLocObject()->logical_index
            << " osIndex=" << hwLocObject()->os_index
            << " size=" << hwLocObject()->attr->cache.size;
        return out.str();
    }

    size_t Cache::size() {
        return hwLocObject()->attr->cache.size;
    }

    std::vector<Core> Cache::cores() {
        std::vector<Core> cores;

        int numCores = hwloc_get_nbobjs_inside_cpuset_by_type(topology(), hwLocObject()->cpuset, HWLOC_OBJ_CORE);
        if (numCores > 0) {
            cores.reserve(numCores);
            hwloc_obj_t current = nullptr;
            for (int i=0; i<numCores; ++i)
            {
                current = hwloc_get_next_obj_inside_cpuset_by_type(topology(), hwLocObject()->cpuset, HWLOC_OBJ_CORE, current);
                cores.push_back(Core(sharedTopology(), current));
            }
        }
        else {
            // no cores found
        }
        return cores;
    }


    // ------------------------------------------------------------------------
    HwLoc::HwLoc() {
        hwloc_topology_t tmpPtr;
        if (0 != hwloc_topology_init(&tmpPtr)) return;

        setTopology(tmpPtr);

        hwloc_topology_ignore_type_keep_structure(topology(), HWLOC_OBJ_PCI_DEVICE);
        hwloc_topology_ignore_type_keep_structure(topology(), HWLOC_OBJ_OS_DEVICE);
        hwloc_topology_ignore_type_keep_structure(topology(), HWLOC_OBJ_BRIDGE);
        hwloc_topology_ignore_type_keep_structure(topology(), HWLOC_OBJ_MISC);

        if (0 != hwloc_topology_load(topology())) setTopology(nullptr);

    }

    std::vector<Cache> HwLoc::getCaches(uint32_t level) {
        std::vector<Cache> caches;

        if (!initialized()) return caches;

        int depth = hwloc_get_cache_type_depth(topology(), level, HWLOC_OBJ_CACHE_DATA);
        unsigned numberOfCaches = 0;
        if (depth == HWLOC_TYPE_DEPTH_UNKNOWN) {
            numberOfCaches = 0;
        } else {
            numberOfCaches = hwloc_get_nbobjs_by_depth(topology(), depth);
            caches.reserve(numberOfCaches);

            for (size_t i=0; i<numberOfCaches; ++i) {
                caches.push_back(Cache(sharedTopology(), hwloc_get_obj_by_depth(topology(), depth, i)));
            }
        }
        return caches;
    }

    size_t HwLoc::getNumberOfCores() {
        if (!initialized()) return 1;

        int depth = hwloc_get_type_depth(topology(), HWLOC_OBJ_CORE);
        unsigned numberOfCores = 0;
        if (depth == HWLOC_TYPE_DEPTH_UNKNOWN) {
            numberOfCores = 0;
        } else {
            numberOfCores = hwloc_get_nbobjs_by_depth(topology(), depth);
        }
        return numberOfCores;
    }

    void HwLoc::distriburtOverCpus(size_t numThreads) {
        if (!initialized()) return;

        hwloc_cpuset_t distributedCpus[numThreads];
        hwloc_obj_t roots[1] = {hwloc_get_root_obj(topology())};
        hwloc_distrib(topology(), roots, 1, distributedCpus, numThreads, INT_MAX, 0);

        char buffer[1024];
        for (size_t i = 0; i < numThreads; ++i) {
            hwloc_bitmap_snprintf(buffer, sizeof(buffer), distributedCpus[i]);
            std::cout << "CPU " << i << " : " << buffer << std::endl;
        }
    }

    bool HwLoc::initialized() const {
        return isTopologySet();
    }
}
