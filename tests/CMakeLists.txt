add_executable(RLyehPnakoticTests
  test_framework.h
  test_main.cpp
  version_tests.cpp
  rlyehpsx_policy_tests.cpp
  pnakotic_policy_tests.cpp
)

target_link_libraries(RLyehPnakoticTests PRIVATE RLyeh::Pnakotic)
target_include_directories(RLyehPnakoticTests PRIVATE "${PNAKOTIC_GENERATED_DIR}")

add_test(NAME RLyehPnakoticTests COMMAND RLyehPnakoticTests)
