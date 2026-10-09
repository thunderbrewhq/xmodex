set(OGG_SRC_DIR "./src")
set(OGG_INCLUDE_DIR "./include")

add_library(ogg-1.1.3 STATIC
    ${OGG_SRC_DIR}/framing.c
    ${OGG_SRC_DIR}/bitwise.c
)

target_include_directories(ogg-1.1.3
    PRIVATE
        ${OGG_SRC_DIR}
    PUBLIC
        ${OGG_INCLUDE_DIR}
)
