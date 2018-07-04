add_library(vinbero_tcp MODULE vinbero_tcp.c)
set_target_properties(vinbero_tcp PROPERTIES PREFIX "")
set_target_properties(vinbero_tcp PROPERTIES 
                                  VERSION 0.1.0
                                  SOVERSION 0.1
                                  PUBLIC_HEADER "")
install(TARGETS vinbero_tcp
        LIBRARY DESTINATION lib/vinbero
        PUBLIC_HEADER DESTINATION include/vinbero)
