cc_library(
  name = "io",
  srcs = ["src/io.hpp"],
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
