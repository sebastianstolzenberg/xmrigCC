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

std::string HwLocObject::toString() const {
    std::ostringstream out;
    out <<  "L#"  << hwLocObject()->logical_index
        << " OS#" << hwLocObject()->os_index
        << " MEM" << hwLocObject()->memory.local_memory
        << " NAM" << hwLocObject()->name;
    return out.str();
}

hwloc_obj_t HwLocObject::hwLocObject() {
    return m_hwLocObject;
};


const hwloc_obj_t HwLocObject::hwLocObject() const {
    return m_hwLocObject;
}

template <class C>
std::vector<C> HwLocObject::getContained(hwloc_obj_type_t type)
{
    std::vector<C> component;

    int numProcessingUnits = hwloc_get_nbobjs_inside_cpuset_by_type(topology(), hwLocObject()->cpuset, type);
    if (numProcessingUnits > 0) {
        component.reserve(numProcessingUnits);
        hwloc_obj_t current = nullptr;
        for (int i=0; i<numProcessingUnits; ++i)
        {
            current = hwloc_get_next_obj_inside_cpuset_by_type(topology(), hwLocObject()->cpuset, type, current);
            component.push_back(C(sharedTopology(), current));
        }
    }
    else {
        // no cores found
    }
    return component;
}

// ------------------------------------------------------------------------
ProcessingUnit::ProcessingUnit(const Topology &topo, hwloc_obj_t hwLocObject) :
        HwLocObject(topo, hwLocObject) {
}

std::string ProcessingUnit::toString() const {
    std::ostringstream out;
    out << "ProcessingUnit " << HwLocObject::toString();
    return out.str();
}

// ------------------------------------------------------------------------
Core::Core(const Topology &topo, hwloc_obj_t hwLocObject) :
        HwLocObject(topo, hwLocObject) {
}

std::string Core::toString() const {
    std::ostringstream out;
    out << "Core " << HwLocObject::toString();
    return out.str();
}

std::vector<ProcessingUnit> Core::processingUnits() {
    return getContained<ProcessingUnit>(HWLOC_OBJ_PU);
}

// ------------------------------------------------------------------------
Cache::Cache(const Topology &topo, hwloc_obj_t hwLocObject) :
        HwLocObject(topo, hwLocObject) {
}

std::string Cache::toString() const {
    std::ostringstream out;
    out << "Cache " << HwLocObject::toString()
        << " size=" << size();
    return out.str();
}

size_t Cache::size() const{
    return hwLocObject()->attr->cache.size;
}

std::vector<Core> Cache::cores() {
    return getContained<Core>(HWLOC_OBJ_CORE);
}

std::vector<ProcessingUnit> Cache::processingUnits() {
    return getContained<ProcessingUnit>(HWLOC_OBJ_PU);
}


// ------------------------------------------------------------------------
Root::Root(const Topology &topo, hwloc_obj_t hwLocObject) :
        HwLocObject(topo, hwLocObject) {
}

std::string Root::toString() const {
    std::ostringstream out;
    out << "Root " << HwLocObject::toString();
    return out.str();
}

std::vector<Core> Root::cores() {
    return getContained<Core>(HWLOC_OBJ_CORE);
}

std::vector<ProcessingUnit> Root::processingUnits() {
    return getContained<ProcessingUnit>(HWLOC_OBJ_PU);
}

std::vector<Cache> Root::caches(uint32_t level) {
    std::vector<Cache> caches;
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

Root HwLoc::root()
{
    return Root(sharedTopology(), hwloc_get_root_obj(topology()));
}

std::vector<Cache> HwLoc::getCaches(uint32_t level) {
    return root().caches(level);
}

std::vector<Core> HwLoc::getCores() {
    return root().cores();
}

std::vector<ProcessingUnit> HwLoc::getProcessingUnits() {
    return root().processingUnits();
}

void HwLoc::distributeOverCpus(size_t numThreads) {
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
