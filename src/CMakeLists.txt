add_library(vinbero_tcp MODULE vinbero_tcp.c)
set_target_properties(vinbero_tcp PROPERTIES PREFIX "")
set_target_properties(vinbero_tcp PROPERTIES VERSION ${vinbero_tcp_version}
                                             SOVERSION ${vinbero_tcp_soversion}
                                             PUBLIC_HEADER "")
find_library(fastdl libfastdl.so REQUIRED)
find_library(gaio libgaio.so.0.3 REQUIRED)
find_library(vinbero_common libvinbero_common.so REQUIRED)
target_link_libraries(vinbero_tcp ${vinbero_common} ${fastdl} ${gaio})

install(TARGETS vinbero_tcp
        LIBRARY DESTINATION lib/vinbero
        PUBLIC_HEADER DESTINATION include/vinbero)
