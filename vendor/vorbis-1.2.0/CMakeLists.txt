set(VORBIS_SRC_DIR "./lib")
set(VORBIS_INCLUDE_DIR "./include")

add_library(vorbis-1.2.0 STATIC
    ${VORBIS_SRC_DIR}/mdct.c
    ${VORBIS_SRC_DIR}/smallft.c
    ${VORBIS_SRC_DIR}/block.c
    ${VORBIS_SRC_DIR}/envelope.c
    ${VORBIS_SRC_DIR}/window.c
    ${VORBIS_SRC_DIR}/lsp.c
    ${VORBIS_SRC_DIR}/lpc.c
    ${VORBIS_SRC_DIR}/analysis.c
    ${VORBIS_SRC_DIR}/synthesis.c
    ${VORBIS_SRC_DIR}/info.c
    ${VORBIS_SRC_DIR}/floor1.c
    ${VORBIS_SRC_DIR}/floor0.c
    ${VORBIS_SRC_DIR}/res0.c
    ${VORBIS_SRC_DIR}/mapping0.c
    ${VORBIS_SRC_DIR}/registry.c
    ${VORBIS_SRC_DIR}/codebook.c
    ${VORBIS_SRC_DIR}/sharedbook.c
    ${VORBIS_SRC_DIR}/lookup.c
    ${VORBIS_SRC_DIR}/bitrate.c
    ${VORBIS_SRC_DIR}/psy.c
    ${VORBIS_SRC_DIR}/vorbisfile.c
)

target_include_directories(vorbis-1.2.0
    PRIVATE
        ${VORBIS_SRC_DIR}
    PUBLIC
        ${VORBIS_INCLUDE_DIR}
)

target_link_libraries(vorbis-1.2.0
    PRIVATE
        ogg-1.1.3
)
