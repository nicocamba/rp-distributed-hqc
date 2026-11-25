cmake_minimum_required(VERSION 3.20)
project(avss_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_path(NTL_INCLUDE_DIR NAMES NTL/ZZ_p.h REQUIRED)
find_library(NTL_LIBRARY NAMES ntl NTL REQUIRED)
find_library(GMP_LIBRARY NAMES gmp REQUIRED)

add_executable(avss_demo
    main.cpp
    src/field.cpp
    src/polynomial.cpp
    src/merkle.cpp
    src/proximity_proof.cpp
    src/deal.cpp
    src/getshare.cpp
)

target_include_directories(avss_demo PRIVATE
    include
    ${NTL_INCLUDE_DIR}
)

target_link_libraries(avss_demo PRIVATE
    ${NTL_LIBRARY}
    ${GMP_LIBRARY}
)

if(WIN32)
    target_link_libraries(avss_demo PRIVATE bcrypt)
endif()

if(MSVC)
    target_compile_options(avss_demo PRIVATE /W4)
else()
    target_compile_options(avss_demo PRIVATE -Wall -Wextra)
endif()
