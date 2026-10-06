add_executable(cymatica_game
    main.cpp
)

target_link_libraries(cymatica_game
    PRIVATE
        raylib
        cymatica_audio
        cymatica_compile_options
)
