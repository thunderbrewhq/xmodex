file(GLOB XMODEX_SOURCES
    "*.cpp"
    "channel/*.cpp"
    "codec/*.cpp"
    "file/*.cpp"
    "dsp/*.cpp"
    "dsp/resampler/*.cpp"
    "os/*.cpp"
    "output/*.cpp"
    "sound/*.cpp"
)

if(XMODEX_SYSTEM_LINUX)
    file(GLOB XMODEX_LINUX_SOURCES
        "os/posix/*.cpp"
        "os/linux/*.cpp"
        "output/bsd/OutputOSS.cpp"
        "output/linux/*.cpp"
    )
    list(APPEND XMODEX_SOURCES ${XMODEX_LINUX_SOURCES})
endif()

if(XMODEX_SYSTEM_MAC)
    file(GLOB XMODEX_MAC_SOURCES
        "os/posix/*.cpp"
        "os/mac/*.cpp"
        "output/mac/*.cpp"
    )
    list(APPEND XMODEX_SOURCES ${XMODEX_MAC_SOURCES})
endif()

if(XMODEX_SYSTEM_BSD)
    file(GLOB XMODEX_BSD_SOURCES
        "os/posix/*.cpp"
        "os/bsd/*.cpp"
        "output/bsd/*.cpp"
    )
    list(APPEND XMODEX_SOURCES ${XMODEX_BSD_SOURCES})
endif()

if(XMODEX_SYSTEM_WIN)
    file(GLOB XMODEX_WIN_SOURCES
        "os/win/*.cpp"
        "output/win/*.cpp"
    )
    list(APPEND XMODEX_SOURCES ${XMODEX_WIN_SOURCES})
endif()

if(XMODEX_COMPAT)
    list(APPEND XMODEX_SOURCES "compat/Compat.cpp")
endif()

if(XMODEX_SHARED)

add_library(fmod SHARED
    ${XMODEX_SOURCES}
)

else()

add_library(fmod STATIC
    ${XMODEX_SOURCES}
)

endif()


# add '${PROJECT_SOURCE_DIR}/include' directory for this
# library and all who link against it

target_include_directories(fmod
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

# add project root to include directories for
# xmodex source files, but not for users of the
# xmodex library

target_include_directories(fmod
    PRIVATE
        ${PROJECT_SOURCE_DIR}
)

# add vendored third-party dependencies

target_link_libraries(fmod
    PRIVATE
        vorbis-1.2.0
        ogg-1.1.3
)

# link unix libraries

if(XMODEX_SYSTEM_LINUX OR XMODEX_SYSTEM_BSD)
    target_link_libraries(fmod
        PRIVATE
            pthread
    )
endif()

# link windows libraries

if(XMODEX_SYSTEM_WIN)
    target_link_libraries(fmod
        PRIVATE
            winmm
    )
endif()

# link osx libraries

if(XMODEX_SYSTEM_MAC)
    target_link_libraries(fmod
        PRIVATE
            "-framework CoreAudio"
            "-framework AudioUnit"
    )
endif()

# shut up about sprintf on mac

if(XMODEX_SYSTEM_MAC)
    target_compile_options(fmod
        PRIVATE 
            -Wno-deprecated-declarations
    )
endif()

# if we wish to distribute xmodex as a shared library,
# link our standard libraries statically to avoid DLL hell

if(XMODEX_SHARED)
    target_link_options(fmod PRIVATE "-static")
endif()
