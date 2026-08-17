add_executable(RLyehPnakoticTests
  test_framework.h
  version_tests.cpp
)

target_link_libraries(RLyehPnakoticTests PRIVATE RLyeh::Pnakotic)

add_test(NAME RLyehPnakoticTests COMMAND RLyehPnakoticTests)
