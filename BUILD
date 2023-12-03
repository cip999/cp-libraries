cc_library(
  name = "common",
  srcs = ["src/common.hpp"],
)

cc_library(
  name = "io",
  srcs = ["src/io.hpp"],
  deps = [":common"],
)

cc_library(
  name = "validation",
  srcs = ["src/validation.hpp"],
  deps = [":common"],
)

cc_test(
  name = "io_test",
  size = "small",
  srcs = ["tests/io_test.cpp"],
  deps = [
    "@com_google_googletest//:gtest_main",
    ":io",
  ],
)
