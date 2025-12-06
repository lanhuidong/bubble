qt_add_executable(${PROJECT_NAME}
    mainwindow.ui
    mainwindow.cpp
    main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)