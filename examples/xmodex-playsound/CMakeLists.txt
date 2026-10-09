add_executable(xmodex-playsound
    main.cpp)

if(XMODEX_SYSTEM_WIN)
    target_link_options(xmodex-playsound
        PRIVATE
            -static
        )
endif()

target_link_libraries(xmodex-playsound PRIVATE fmod)
