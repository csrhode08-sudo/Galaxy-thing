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
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    # clang-cl (Windows): same semantics as the GNU-style build; wchar_t is already 16-bit and templates are
    # parsed late by default.
    add_compile_options(
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wno-multichar>
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-trigraphs>
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-declarations>
        $<$<COMPILE_LANGUAGE:C,CXX>:/clang:-fno-strict-aliasing>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wno-c++11-narrowing>
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-register>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wno-microsoft-cast>
        $<$<COMPILE_LANGUAGE:C,CXX>:/EHsc>
        $<$<COMPILE_LANGUAGE:C,CXX>:/utf-8>
    )
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX)
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
# The CPU DSP mixer is built with JSystem (it uses JAudio2's headers and the game prelude).
file(GLOB PC_AUDIO_SRC CONFIGURE_DEPENDS pc/src/audio/*.cpp)
list(FILTER PC_SRC EXCLUDE REGEX "pc/src/audio/")
list(APPEND JSYSTEM_SRC ${PC_AUDIO_SRC})
add_subdirectory(pc/third_party/freeverb EXCLUDE_FROM_ALL)

# Console-only sources replaced by the PC platform layer.
list(FILTER JSYSTEM_SRC EXCLUDE REGEX "JAudio2/(dsptask|osdsp|osdsp_task|dspproc)\\.cpp$")

# String literals in Shift-JIS, like the original CodeWarrior build: the game looks things up by Japanese names
# (object, demo, camera, animation and rumble names) that come from the disc in Shift-JIS. Files with such literals
# are compiled from converted copies in ${CMAKE_BINARY_DIR}/sjis (tools/sjis_literals.py).
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(SJIS_TOOL ${CMAKE_SOURCE_DIR}/tools/sjis_literals.py)
set(SJIS_DIR ${CMAKE_BINARY_DIR}/sjis)
execute_process(COMMAND ${Python3_EXECUTABLE} ${SJIS_TOOL} --scan ${CMAKE_SOURCE_DIR} src include libs
                OUTPUT_VARIABLE SJIS_FILES RESULT_VARIABLE _sjis_rc)
if (NOT _sjis_rc EQUAL 0)
    message(FATAL_ERROR "sjis_literals.py --scan failed")
endif ()
set(SJIS_OUTPUTS)
foreach (rel ${SJIS_FILES})
    set(_out ${SJIS_DIR}/${rel})
    add_custom_command(OUTPUT ${_out}
                       COMMAND ${Python3_EXECUTABLE} ${SJIS_TOOL} ${CMAKE_SOURCE_DIR}/${rel} ${_out}
                       DEPENDS ${CMAKE_SOURCE_DIR}/${rel} ${SJIS_TOOL}
                       COMMENT "Shift-JIS literals: ${rel}" VERBATIM)
    list(APPEND SJIS_OUTPUTS ${_out})
    if (NOT EXISTS ${_out})
        # Headers must exist before the first compile scans for them.
        execute_process(COMMAND ${Python3_EXECUTABLE} ${SJIS_TOOL} ${CMAKE_SOURCE_DIR}/${rel} ${_out})
    endif ()
endforeach ()
add_custom_target(sjis_literals DEPENDS ${SJIS_OUTPUTS})
# Converted headers shadow the originals (after the PC shims in pc/include).
list(INSERT GAME_INCLUDES 1
    ${SJIS_DIR}/include
    ${SJIS_DIR}/libs/JSystem/include
    ${SJIS_DIR}/libs/nw4r/include
    ${SJIS_DIR}/libs/RVLFaceLib/include
    ${SJIS_DIR}/libs/RVL_SDK/include
)
foreach (_list GAME_SRC JSYSTEM_SRC NW4R_SRC)
    set(_new)
    foreach (src ${${_list}})
        file(RELATIVE_PATH rel ${CMAKE_SOURCE_DIR} ${src})
        if (rel IN_LIST SJIS_FILES)
            get_filename_component(_dir ${src} DIRECTORY)
            set_source_files_properties(${SJIS_DIR}/${rel} PROPERTIES INCLUDE_DIRECTORIES ${_dir})
            list(APPEND _new ${SJIS_DIR}/${rel})
        else ()
            list(APPEND _new ${src})
        endif ()
    endforeach ()
    set(${_list} ${_new})
endforeach ()

set(AURORA_LIBS aurora::core aurora::gx aurora::gd aurora::vi aurora::pad aurora::mtx aurora::os aurora::dvd aurora::thp aurora::main)

# Game code: C++ semantics close to CodeWarrior's (no strict two-phase lookup, 16-bit wchar_t).
if (MSVC)
    # /vmg /vmv: one pointer-to-member-function layout everywhere. By default MSVC sizes them by the class's
    # inheritance, which differs between translation units that see a class only forward-declared and ones that
    # see its definition, so calls through them jump to garbage.
    set(GAME_CXX_FLAGS /clang:-Werror=return-type /vmg /vmv)
else ()
    set(GAME_CXX_FLAGS -fdelayed-template-parsing -fshort-wchar -Werror=return-type)
endif ()

foreach (lib game jsystem nw4r pc)
    string(TOUPPER ${lib} _u)
    add_library(smg_${lib} OBJECT ${${_u}_SRC})
    target_include_directories(smg_${lib} PRIVATE ${GAME_INCLUDES})
    target_compile_definitions(smg_${lib} PRIVATE ${GAME_DEFS})
    target_compile_options(smg_${lib} PRIVATE ${GAME_CXX_FLAGS})
    target_link_libraries(smg_${lib} PRIVATE ${AURORA_LIBS})
    add_dependencies(smg_${lib} sjis_literals)
endforeach ()
if (MSVC)
    target_compile_options(smg_pc PRIVATE "/FI${CMAKE_SOURCE_DIR}/pc/include/pc/win_compat.h")
endif ()
foreach (lib game jsystem nw4r)
    if (MSVC)
        target_compile_options(smg_${lib} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/FI${CMAKE_SOURCE_DIR}/pc/include/pc/win_compat.h>")
        target_compile_options(smg_${lib} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/FI${CMAKE_SOURCE_DIR}/pc/include/pc/prelude.hpp>")
    else ()
        target_compile_options(smg_${lib} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:SHELL:-include ${CMAKE_SOURCE_DIR}/pc/include/pc/prelude.hpp>")
    endif ()
endforeach ()

target_link_libraries(smg_jsystem PRIVATE freeverb)
add_executable(galaxy $<TARGET_OBJECTS:smg_game> $<TARGET_OBJECTS:smg_jsystem> $<TARGET_OBJECTS:smg_nw4r> $<TARGET_OBJECTS:smg_pc>)
target_link_libraries(galaxy PRIVATE ${AURORA_LIBS} freeverb)
if (WIN32)
    set_target_properties(galaxy PROPERTIES OUTPUT_NAME "SuperMarioGalaxy" WIN32_EXECUTABLE OFF)
    # The game deliberately redefines some JSystem functions (Overwrite*.cpp). ELF resolves these as
    # weak symbols with the first (game) definition winning; COFF needs to be told to do the same.
    target_link_options(galaxy PRIVATE "/FORCE:MULTIPLE" "/errorlimit:0")
    # Linux gives threads 8 MB of stack; Windows defaults to 1 MB, which the game overruns. Reserve 32 MB
    # (also the default for every thread the game creates).
    target_link_options(galaxy PRIVATE "/STACK:33554432")
endif ()
