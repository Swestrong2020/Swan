project(SwanMain)

add_subdirectory(Swan)

add_executable(main
    main.c
)

target_link_libraries(main m swan)