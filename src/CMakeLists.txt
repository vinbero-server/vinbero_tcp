add_library(tucube_tcp MODULE tucube_tcp.c)
set_target_properties(tucube_tcp PROPERTIES PREFIX "")
set_target_properties(tucube_tcp PROPERTIES PUBLIC_HEADER "")
install(TARGETS tucube_tcp
        LIBRARY DESTINATION lib/tucube
        PUBLIC_HEADER DESTINATION include/tucube)
