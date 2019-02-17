find_package(fastdl REQUIRED)
find_package(gaio REQUIRED)
find_package(vinbero_com REQUIRED)

add_library(vinbero_tcp MODULE)
target_sources(vinbero_tcp PRIVATE vinbero_tcp.c)
target_compile_options(vinbero_tcp PRIVATE -Wall)
set_target_properties(
    vinbero_tcp
    PROPERTIES
    C_STANDARD 11
    PREFIX ""
    VERSION ${version}
    SOVERSION ${soversion}
    PUBLIC_HEADER ""
)

target_include_directories(
    vinbero_tcp
    PRIVATE
    ${PROJECT_BINARY_DIR}
)

target_link_libraries(
    vinbero_tcp
    vinbero_com::vinbero_com
    fastdl::fastdl
    gaio::gaio
)

configure_file(
    "${PROJECT_SOURCE_DIR}/src/config.h.in"
    "${PROJECT_BINARY_DIR}/config.h"
    @ONLY
)

install(
    TARGETS vinbero_tcp
    LIBRARY DESTINATION lib/vinbero
    PUBLIC_HEADER DESTINATION include/vinbero
)
