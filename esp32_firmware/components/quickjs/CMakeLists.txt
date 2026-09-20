idf_component_register(
    SRCS
        quickjs/quickjs.c
        quickjs/libregexp.c
        quickjs/libunicode.c
        quickjs/dtoa.c
    INCLUDE_DIRS
        quickjs
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE _GNU_SOURCE)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -fwrapv
    -Wno-implicit-fallthrough
    -Wno-unused-parameter
    -Wno-sign-compare
    -Wno-missing-field-initializers
    -Wno-unused-function
    -Wno-format
)