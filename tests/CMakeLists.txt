file(GLOB_RECURSE SELF_TESTS_CPP RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")
qi_create_lib(self_tests SHARED ${SELF_TESTS_CPP})
qi_use_lib(self_tests utils self)
qi_stage_lib(self_tests)

