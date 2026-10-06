add_executable(cymatica_game
    main.cpp
)

target_link_libraries(cymatica_game
    PRIVATE
        raylib
        cymatica_core
        cymatica_audio
        cymatica_replay
        cymatica_compile_options
)
