cmake_minimum_required(VERSION 3.20)
project(avss_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_path(NTL_INCLUDE_DIR NAMES NTL/ZZ_p.h REQUIRED)
find_library(NTL_LIBRARY NAMES ntl NTL REQUIRED)
find_library(GMP_LIBRARY NAMES gmp REQUIRED)

set(AVSS_CORE_SOURCES
    src/math/field.cpp
    src/math/polynomial.cpp
    src/crypto/merkle.cpp
    src/proofs/proximity_proof.cpp
    src/protocol/avss/deal.cpp
    src/protocol/avss/getshare.cpp
)

add_library(avss_core STATIC ${AVSS_CORE_SOURCES})

target_include_directories(avss_core PUBLIC
    include
    ${NTL_INCLUDE_DIR}
)

target_link_libraries(avss_core PUBLIC
    ${NTL_LIBRARY}
    ${GMP_LIBRARY}
)

if(WIN32)
    target_link_libraries(avss_core PUBLIC bcrypt)
endif()

add_executable(avss_demo apps/avss_main.cpp)
target_link_libraries(avss_demo PRIVATE avss_core)

add_executable(dkg_demo apps/dkg_main.cpp src/protocol/dkg/dkg.cpp)
target_link_libraries(dkg_demo PRIVATE avss_core)

foreach(target avss_core avss_demo dkg_demo)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
endforeach()
