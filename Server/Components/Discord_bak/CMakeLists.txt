get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_server_component(${ProjectId})

target_compile_definitions(${ProjectId} PRIVATE
    CPPHTTPLIB_OPENSSL_SUPPORT
)

target_link_libraries(${ProjectId} PRIVATE
    liblua
    CONAN_PKG::openssl
    CONAN_PKG::nlohmann_json
    CONAN_PKG::ghc-filesystem
)

target_include_directories(${ProjectId} PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/lua/src
    ${CMAKE_SOURCE_DIR}/lib/cpp-httplib
)
