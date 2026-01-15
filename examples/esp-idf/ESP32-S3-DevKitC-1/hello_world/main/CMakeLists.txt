idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES mbedtls libtropic
)

# Propagate selected SH0 key macros into this component's compile definitions.
# Note for users: if you are writing your own application, the following logic
# with LT_SH0_KEYS is not necessary - use specific key pair, e.g. your own or 
# the ones from libtropic_common.h.
if(NOT DEFINED LT_USE_SH0_ENG_SAMPLE)
    set(LT_USE_SH0_ENG_SAMPLE 0)
endif()
if(NOT DEFINED LT_USE_SH0_PROD0)
    set(LT_USE_SH0_PROD0 0)
endif()
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    LT_USE_SH0_ENG_SAMPLE=${LT_USE_SH0_ENG_SAMPLE}
    LT_USE_SH0_PROD0=${LT_USE_SH0_PROD0}
)
