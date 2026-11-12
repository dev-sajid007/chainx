add_executable(chainx_tests
    unit/main.cpp
    unit/test_urlutils.cpp
    unit/test_database.cpp)

target_link_libraries(chainx_tests PRIVATE chainx_core GTest::gtest)

add_test(NAME chainx_tests COMMAND chainx_tests)

add_executable(chainx_ui_tests
    ui/main.cpp
    ui/test_dialogs.cpp)

target_link_libraries(chainx_ui_tests PRIVATE chainx_core GTest::gtest)

add_test(NAME chainx_ui_tests COMMAND chainx_ui_tests)
set_tests_properties(chainx_ui_tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
