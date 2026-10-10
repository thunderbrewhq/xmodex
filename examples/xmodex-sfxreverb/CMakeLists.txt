add_executable(xmodex-sfxreverb
    main.cpp)

if(XMODEX_SYSTEM_WIN)
    target_link_options(xmodex-sfxreverb
        PRIVATE
            -static
        )
endif()

target_link_libraries(xmodex-sfxreverb PRIVATE fmod)
