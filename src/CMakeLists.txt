add_library(vinbero_tcp MODULE)
target_sources(vinbero_tcp PRIVATE vinbero_tcp.c)

set_target_properties(
    vinbero_tcp
    PROPERTIES
    C_STANDARD 11
    PREFIX ""
    VERSION ${version}
    SOVERSION ${soversion}
    PUBLIC_HEADER ""
)

find_library(fastdl libfastdl.so REQUIRED)
find_library(gaio libgaio.so REQUIRED)
find_library(vinbero_com libvinbero_com.so REQUIRED)
target_link_libraries(vinbero_tcp ${vinbero_com} ${fastdl} ${gaio})

install(
    TARGETS vinbero_tcp
    LIBRARY DESTINATION lib/vinbero
    PUBLIC_HEADER DESTINATION include/vinbero
)
