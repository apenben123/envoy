load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_extension",
    "envoy_extension_package",
)

envoy_extension_package()

# envoy_cc_library​​
# ​​用途​​：定义 Envoy 的 ​​通用 C++ 库​​。
# ​​特点​​：
# 封装了 Bazel 原生的 cc_library，并添加 Envoy 项目特定的默认配置（如编译器标志、依赖项）。
# 适用于所有不需要注册为 Envoy 扩展（如过滤器、访问日志器等）的普通 C++ 库。


# envoy_cc_extension​​
# ​​用途​​：定义 Envoy 的 ​​扩展模块​​（如 HTTP 过滤器、网络过滤器、访问日志器等）。
# ​​特点​​：
# 在 envoy_cc_library 的基础上，添加了扩展注册逻辑和 Envoy 扩展 API 的依赖。
# 自动处理扩展的静态注册（通过 REGISTER_FACTORY 宏）。
# 强制启用 RTTI（运行时类型信息），因为扩展的动态分发需要类型识别。


### 核心区别
# |   特性    | `envoy_cc_library`              | `envoy_cc_extension`                   |
# | :------: | :------------------------------- | :------------------------------------- |
# | 目标类型  | 通用 C++ 库                      | Envoy 扩展（过滤器、日志器等）            |
# | RTTI 支持 | 默认禁用（除非显式启用）          | 强制启用                                 |
# | 扩展注册  | 无                              | 自动处理静态注册逻辑                       |
# |  依赖项   | 基础依赖（如 Abseil、Protobuf）  | 扩展 API 依赖（如 `//source/extensions`） |
# | 典型场景  | 工具类、数据结构、通用逻辑        | HTTP 过滤器、监听器扩展、访问日志          |

envoy_cc_extension(
    name = "config",
    srcs = [
        "config.cc",
        "hyperscan_scanner.cc",
    ],
    hdrs = [
        "config.h",
        "hyperscan_scanner.h",
    ],
    # repository = "@envoy",
    deps = [
        "//envoy/http:filter_interface",
        "//envoy/registry:registry",
        "//envoy/server:filter_config_interface", 
        "//source/common/common:assert_lib",
        "//source/common/protobuf:utility_lib",
        "@hyperscan//:libhs",  # 需要添加 hyperscan 依赖
        "@envoy_api//envoy/extensions/filters/http/hyperscan_scanner/v3:pkg_cc_proto", # 添加 proto 依赖
    ],
)