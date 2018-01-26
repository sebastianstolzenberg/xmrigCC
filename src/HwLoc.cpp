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

    //TODO check for mem leaks -> hwloc_bitmap etc.

// ------------------------------------------------------------------------
TopologySharer::TopologySharer()
{
}

TopologySharer::TopologySharer(const Topology &topo) :
    m_topology(topo)
{
}

void TopologySharer::setTopology(hwloc_topology_t topo)
{
    m_topology = Topology(topo, hwloc_topology_destroy);
}

bool TopologySharer::isTopologySet() const {
    return static_cast<bool>(m_topology);
}

Topology TopologySharer::sharedTopology()
{
    return m_topology;
}

hwloc_topology_t TopologySharer::topology()
{
    return m_topology.get();
}

// ------------------------------------------------------------------------
CpuSet::CpuSet(const Topology &topo, hwloc_cpuset_t cpuSet) :
        TopologySharer(topo),
        m_hwLocCpuSet(cpuSet)
{
    if (cpuSet == nullptr) {
        std::cout << "ERR: cpuSet is null";
    }
}

void CpuSet::bindThread()
{
    //TODO test with HWLOC_CPUBIND_STRICT
    hwloc_set_cpubind(topology(), m_hwLocCpuSet, HWLOC_CPUBIND_THREAD);
}

void* CpuSet::allocMemBind(size_t len)
{
    return hwloc_alloc_membind(topology(), len, m_hwLocCpuSet, HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_THREAD);
}

// ------------------------------------------------------------------------
HwLocObject::HwLocObject(const Topology &topo, hwloc_obj_t hwLocObject) :
        TopologySharer(topo),
        m_hwLocObject(hwLocObject)
{
    if (hwLocObject == nullptr) {
      std::cout << "ERR: hwLocObject is null";
    }
}

std::string HwLocObject::toString() const {
    std::ostringstream out;
    std::string hwlocString(500, ' ');
    hwloc_obj_attr_snprintf(&hwlocString[0], hwlocString.size(), hwLocObject(), "\n", 5);
    out << " snprintf, " << hwlocString << std::endl;
    for (unsigned i=0; i<hwLocObject()->infos_count; ++i) {
        out << "Info: " << hwLocObject()->infos[i].name << "=" << hwLocObject()->infos[i].value << std::endl;
    }
    out <<  "L#"  << hwLocObject()->logical_index
        << " OS#" << hwLocObject()->os_index
        << " MEM" << hwLocObject()->memory.local_memory
        << " NAM" << hwLocObject()->name;
    return out.str();
}

hwloc_obj_t HwLocObject::hwLocObject()
{
    return m_hwLocObject;
};


const hwloc_obj_t HwLocObject::hwLocObject() const {
    return m_hwLocObject;
}

CpuSet HwLocObject::cpuSet()
{
    return CpuSet(sharedTopology(), hwLocObject()->cpuset);
}

size_t  HwLocObject::getNumContained(hwloc_obj_type_t type)
{
    int numContained = hwloc_get_nbobjs_inside_cpuset_by_type(topology(), hwLocObject()->cpuset, type);
    return std::max(numContained, 0);
}

template <class C>
std::vector<C> HwLocObject::getContained(hwloc_obj_type_t type)
{
    std::vector<C> component;

    int numContained = hwloc_get_nbobjs_inside_cpuset_by_type(topology(), hwLocObject()->cpuset, type);
    if (numContained > 0) {
        component.reserve(numContained);
        hwloc_obj_t current = nullptr;
        for (int i=0; i<numContained; ++i)
        {
            current = hwloc_get_next_obj_inside_cpuset_by_type(topology(), hwLocObject()->cpuset, type, current);
            component.push_back(C(sharedTopology(), current));
        }
    }
    else {
        // none found
    }
    return component;
}

// ------------------------------------------------------------------------
ProcessingUnit::ProcessingUnit(const Topology &topo, hwloc_obj_t hwLocObject) :
        HwLocObject(topo, hwLocObject)
{
}

std::string ProcessingUnit::toString() const {
    std::ostringstream out;
    out << "ProcessingUnit " << HwLocObject::toString();
    return out.str();
}


// ------------------------------------------------------------------------
Core::Core(const Topology &topo, hwloc_obj_t hwLocObject) :
        HwLocObject(topo, hwLocObject)
{
}

std::string Core::toString() const {
    std::ostringstream out;
    out << "Core " << HwLocObject::toString();
    return out.str();
}

std::vector<ProcessingUnit> Core::processingUnits()
{
    return getContained<ProcessingUnit>(HWLOC_OBJ_PU);
}

// ------------------------------------------------------------------------
Cache::Cache(const Topology &topo, hwloc_obj_t hwLocObject) :
        HwLocObject(topo, hwLocObject)
{
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

std::vector<Core> Cache::cores()
{
    return getContained<Core>(HWLOC_OBJ_CORE);
}

std::vector<ProcessingUnit> Cache::processingUnits()
{
    return getContained<ProcessingUnit>(HWLOC_OBJ_PU);
}


// ------------------------------------------------------------------------
Package::Package(const Topology &topo, hwloc_obj_t hwLocObject) : HwLocObject(topo, hwLocObject)
{

}

// ------------------------------------------------------------------------
Root::Root(const Topology &topo, hwloc_obj_t hwLocObject) :
        HwLocObject(topo, hwLocObject)
{
}

std::string Root::toString() const {
    std::ostringstream out;
    out << "Root " << HwLocObject::toString();
    return out.str();
}

size_t Root::numPackages()
{
    return getNumContained(HWLOC_OBJ_PACKAGE);
}

std::vector<Package> Root::packages()
{
    return getContained<Package>(HWLOC_OBJ_PACKAGE);
}

size_t Root::numCaches(uint32_t level)
{
    int depth = hwloc_get_cache_type_depth(topology(), level, HWLOC_OBJ_CACHE_DATA);
    size_t numberOfCaches = 0;
    if (depth == HWLOC_TYPE_DEPTH_UNKNOWN) {
        numberOfCaches = 0;
    } else {
        numberOfCaches = hwloc_get_nbobjs_by_depth(topology(), depth);
    }
    return numberOfCaches;
}

std::vector<Cache> Root::caches(uint32_t level)
{
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

size_t Root::numCores()
{
    return getNumContained(HWLOC_OBJ_CORE);
}

std::vector<Core> Root::cores()
{
    return getContained<Core>(HWLOC_OBJ_CORE);
}

size_t Root::numProcessingUnits()
{
    return getNumContained(HWLOC_OBJ_PU);
}

std::vector<ProcessingUnit> Root::processingUnits()
{
    return getContained<ProcessingUnit>(HWLOC_OBJ_PU);
}

// ------------------------------------------------------------------------
HwLoc::HwLoc()
{
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

size_t HwLoc::getNumPackages()
{
    return root().numPackages();
}

std::vector<Package> HwLoc::getPackages()
{
    return root().packages();
}

size_t HwLoc::getNumCaches(uint32_t level)
{
    return root().numCaches(level);
}

std::vector<Cache> HwLoc::getCaches(uint32_t level)
{
    return root().caches(level);
}

size_t HwLoc::getNumCores()
{
    return root().numCores();
}

std::vector<Core> HwLoc::getCores()
{
    return root().cores();
}

size_t HwLoc::getNumProcessingUnits()
{
    return root().numProcessingUnits();
}

std::vector<ProcessingUnit> HwLoc::getProcessingUnits()
{
    return root().processingUnits();
}

std::vector<CpuSet> HwLoc::getDistributedCpuSets(size_t num)
{
    std::vector<CpuSet> pus;

    if (!initialized()) return pus;

    hwloc_cpuset_t distributedCpus[num];
    hwloc_obj_t roots[1] = {hwloc_get_root_obj(topology())};
    hwloc_distrib(topology(), roots, 1, distributedCpus, num, INT_MAX, 0);

    for (size_t i=0; i<num; ++i) {
        pus.push_back(CpuSet(sharedTopology(), distributedCpus[i]));
    }

    char buffer[1024];
    for (size_t i = 0; i < num; ++i) {
        hwloc_bitmap_snprintf(buffer, sizeof(buffer), distributedCpus[i]);
        std::cout << "CPU " << i << " : " << buffer << std::endl;
    }
}

void HwLoc::distributeOverCpus(size_t numThreads)
{
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

} //namespace hwloc
