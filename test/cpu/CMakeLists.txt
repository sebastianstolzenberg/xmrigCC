set(SOURCES
    cpu.cpp
    ../../src/Cpu.cpp
    ../../src/HwLoc.cpp
    ../../src/
)

include_directories(../../src)

add_executable(cpu_test ${SOURCES})
target_link_libraries(cpu_test unity ${Hwloc_LIBRARIES} ${EXTRA_LIBS})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -maes -fno-strict-aliasing")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")
add_definitions(-DBUILD_TEST)

add_test(cpu_test cpu_test)
