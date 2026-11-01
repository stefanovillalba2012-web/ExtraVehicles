cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(ExtraVehicles VERSION 1.0.0)

file(GLOB SOURCES src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Arreglo: la libreria fmt (que descarga Geode) no compila con Clang 21 (NDK 29)
# por un problema con "consteval". Se desactiva consteval en sus cabeceras antes de compilar.
file(GLOB_RECURSE EV_FMT_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/cpm-cache/fmt/*.h")
foreach(ev_hdr ${EV_FMT_HEADERS})
    file(READ "${ev_hdr}" ev_old)
    string(REPLACE "FMT_USE_CONSTEVAL 1" "FMT_USE_CONSTEVAL 0" ev_new "${ev_old}")
    string(REPLACE "define FMT_CONSTEVAL consteval" "define FMT_CONSTEVAL" ev_new "${ev_new}")
    if (NOT ev_new STREQUAL ev_old)
        file(WRITE "${ev_hdr}" "${ev_new}")
        message(STATUS "Parche consteval aplicado a ${ev_hdr}")
    endif()
endforeach()

setup_geode_mod(${PROJECT_NAME})
