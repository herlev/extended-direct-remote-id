idf_component_register(SRCS "wifi.c" "opendroneid.c"
                       INCLUDE_DIRS ".")
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=format -Wno-format)
