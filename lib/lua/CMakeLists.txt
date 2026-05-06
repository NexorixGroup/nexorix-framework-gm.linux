# Lua 5.4 static library
set(LUA_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(LUA_CORE_SOURCES
    ${LUA_SRC_DIR}/lapi.c
    ${LUA_SRC_DIR}/lcode.c
    ${LUA_SRC_DIR}/lctype.c
    ${LUA_SRC_DIR}/ldebug.c
    ${LUA_SRC_DIR}/ldo.c
    ${LUA_SRC_DIR}/ldump.c
    ${LUA_SRC_DIR}/lfunc.c
    ${LUA_SRC_DIR}/lgc.c
    ${LUA_SRC_DIR}/llex.c
    ${LUA_SRC_DIR}/lmem.c
    ${LUA_SRC_DIR}/lobject.c
    ${LUA_SRC_DIR}/lopcodes.c
    ${LUA_SRC_DIR}/lparser.c
    ${LUA_SRC_DIR}/lstate.c
    ${LUA_SRC_DIR}/lstring.c
    ${LUA_SRC_DIR}/ltable.c
    ${LUA_SRC_DIR}/ltm.c
    ${LUA_SRC_DIR}/lundump.c
    ${LUA_SRC_DIR}/lvm.c
    ${LUA_SRC_DIR}/lzio.c
)

set(LUA_LIB_SOURCES
    ${LUA_SRC_DIR}/lauxlib.c
    ${LUA_SRC_DIR}/lbaselib.c
    ${LUA_SRC_DIR}/lcorolib.c
    ${LUA_SRC_DIR}/ldblib.c
    ${LUA_SRC_DIR}/liolib.c
    ${LUA_SRC_DIR}/lmathlib.c
    ${LUA_SRC_DIR}/loadlib.c
    ${LUA_SRC_DIR}/loslib.c
    ${LUA_SRC_DIR}/lstrlib.c
    ${LUA_SRC_DIR}/ltablib.c
    ${LUA_SRC_DIR}/lutf8lib.c
    ${LUA_SRC_DIR}/linit.c
)

add_library(liblua STATIC
    ${LUA_CORE_SOURCES}
    ${LUA_LIB_SOURCES}
)

target_include_directories(liblua
    PUBLIC ${LUA_SRC_DIR}
)

# Suppress warnings from Lua source
if(MSVC)
    target_compile_options(liblua PRIVATE
        /wd4244  # conversion warnings
        /wd4267  # size_t conversion
        /wd4334  # shift result
        /wd4996  # deprecated functions
    )
else()
    target_compile_options(liblua PRIVATE
        -Wno-unused-function
        -Wno-implicit-fallthrough
    )
endif()

if(WIN32)
    target_compile_definitions(liblua PRIVATE
        _CRT_SECURE_NO_WARNINGS
        LUA_BUILD_AS_DLL=0
    )
endif()

set_property(TARGET liblua PROPERTY FOLDER "lib")
set_property(TARGET liblua PROPERTY POSITION_INDEPENDENT_CODE ON)
