cmake_minimum_required(VERSION 3.25)
project(galaxy_pc LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(AURORA_ENABLE_DVD ON CACHE BOOL "" FORCE)
set(AURORA_ENABLE_CARD ON CACHE BOOL "" FORCE)
set(AURORA_ENABLE_THP ON CACHE BOOL "" FORCE)
set(AURORA_ENABLE_RMLUI ON CACHE BOOL "" FORCE)
add_subdirectory(extern/aurora EXCLUDE_FROM_ALL)
target_compile_definitions(aurora_mtx PRIVATE MTX_USE_PS=1)

if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
    add_compile_options(
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wno-multichar>
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-trigraphs>
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-declarations>
        $<$<COMPILE_LANGUAGE:C,CXX>:-fno-strict-aliasing>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wno-c++11-narrowing>
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-register>
    )
endif ()

set(GAME_DEFS TARGET_PC=1 GALAXY_PC=1 NON_MATCHING=1 MTX_USE_PS=1 RMGJ01=0 RMGE01=1 RMGP01=2 RMGK01=3 VERSION=3 NDEBUG=1)
set(GAME_INCLUDES
    ${CMAKE_SOURCE_DIR}/pc/include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/JSystem/include
    ${CMAKE_SOURCE_DIR}/libs/nw4r/include
    ${CMAKE_SOURCE_DIR}/libs/RVLFaceLib/include
    ${CMAKE_SOURCE_DIR}/libs/RVL_SDK/include
    ${CMAKE_SOURCE_DIR}/extern/aurora/include
    ${CMAKE_SOURCE_DIR}/gen
)

file(GLOB_RECURSE GAME_SRC CONFIGURE_DEPENDS src/Game/*.cpp)
file(GLOB_RECURSE JSYSTEM_SRC CONFIGURE_DEPENDS src/JSystem/*.cpp)
file(GLOB_RECURSE NW4R_SRC CONFIGURE_DEPENDS src/nw4r/*.cpp)
file(GLOB_RECURSE PC_SRC CONFIGURE_DEPENDS pc/src/*.cpp pc/src/*.c)

# Console-only sources replaced by the PC platform layer.
list(FILTER JSYSTEM_SRC EXCLUDE REGEX "JAudio2/(dsptask|osdsp|osdsp_task|dspproc)\\.cpp$")

set(AURORA_LIBS aurora::core aurora::gx aurora::gd aurora::vi aurora::pad aurora::mtx aurora::os aurora::dvd aurora::thp aurora::main)

# Game code: C++ semantics close to CodeWarrior's (no strict two-phase lookup, 16-bit wchar_t).
set(GAME_CXX_FLAGS -fdelayed-template-parsing -fshort-wchar)

foreach (lib game jsystem nw4r pc)
    string(TOUPPER ${lib} _u)
    add_library(smg_${lib} OBJECT ${${_u}_SRC})
    target_include_directories(smg_${lib} PRIVATE ${GAME_INCLUDES})
    target_compile_definitions(smg_${lib} PRIVATE ${GAME_DEFS})
    target_compile_options(smg_${lib} PRIVATE ${GAME_CXX_FLAGS})
    target_link_libraries(smg_${lib} PRIVATE ${AURORA_LIBS})
endforeach ()
foreach (lib game jsystem nw4r)
    target_compile_options(smg_${lib} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:SHELL:-include ${CMAKE_SOURCE_DIR}/pc/include/pc/prelude.hpp>")
endforeach ()

add_executable(galaxy $<TARGET_OBJECTS:smg_game> $<TARGET_OBJECTS:smg_jsystem> $<TARGET_OBJECTS:smg_nw4r> $<TARGET_OBJECTS:smg_pc>)
target_link_libraries(galaxy PRIVATE ${AURORA_LIBS})
if (WIN32)
    set_target_properties(galaxy PROPERTIES OUTPUT_NAME "SuperMarioGalaxy")
endif ()
