load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_test",
    "envoy_package",
)

licenses(["notice"])  # Apache 2

envoy_package()

envoy_cc_test(
    name = "schema_validator_test",
    srcs = ["schema_validator_test.cc"],
    data = [
        ":configs",
    ],
    rbe_pool = "6gig",
    deps = [
        "//test/tools/schema_validator:schema_validator_lib",
    ],
)

filegroup(
    name = "configs",
    srcs = glob([
        "config/*.yaml",
    ]),
)
