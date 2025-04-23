#!/bin/bash

# # 示例用法：
# ./make.sh --clean --filter  # 清理后仅构建过滤器
# ./make.sh --proto --envoy  # 构建Proto和Envoy
# ./make.sh --help
# ./build.sh --clean --filter --envoy # 清理后构建过滤器和Envoy

# 环境配置
BAZEL_CACHE_DIR="${HOME}/.bazel_cache"
PROTO_TARGET="@envoy_api//envoy/extensions/filters/http/hyperscan_scanner/v3:pkg"
FILTER_TARGET="//source/extensions/filters/http/hyperscan_scanner:config"
ENVOY_TARGET="//source/exe:envoy-static"

HALF_CPUS=$(( ($(nproc) + 1) / 2 ))

# 彩色日志函数
log() {
    local color=$1
    local level=$2
    shift 2
    echo -e "\033[${color}m[$(date '+%Y-%m-%d %H:%M:%S')] [${level}]\033[0m $*"
}

# 清理构建环境
clean_build() {
    log 32 INFO "开始清理构建缓存..."
    if ! bazel clean --expunge; then
        log 31 ERROR "清理缓存失败"
        exit 1
    fi
    log 32 INFO "清理完成"
}

# 准备构建环境
prepare_build() {
    log 32 INFO "准备构建环境..."
    export GOPROXY=https://goproxy.cn,direct
    
    log 32 INFO "预下载依赖..."
    if ! bazel fetch //...; then
        log 31 ERROR "依赖下载失败"
        exit 1
    fi
}

# 构建Proto定义
build_proto() {
    log 32 INFO "开始构建Proto定义..."

    # 自动修复proto格式
    log 33 NOTE "正在检查proto格式..."
    # if ! ./tools/proto_format/proto_sync.py --mode fix --api_root ./api; then
    if ! ./tools/proto_format/proto_format.sh fix; then
        log 33 WARN "proto格式修复失败，继续构建..."
    fi

    # ​​先编译Proto定义​​ 编译 ​​Protobuf 定义相关的描述符文件和数据包​​ 
    # 编译目标解析
    # | 组件                                                 |                             说明                            |
    # | :--------------------------------------------------- | :--------------------------------------------------------: |
    # | `@envoy_api//`                                       | 指向 Envoy API 仓库（包含所有Protobuf定义）                  |
    # | `envoy/extensions/filters/http/hyperscan_scanner/v3` | 您的过滤器API版本化路径（v3表示使用Envoy v3 API）             |
    # | `:pkg`                                               | Bazel目标，生成该proto包的**描述符文件**和**语言绑定元数据**   |
    # Bazel 允许在编译时指定 --output_groups= 来获取不同的输出文件
    local output_groups="default,proto_h,proto_cc"
    log 32 INFO "编译目标：${PROTO_TARGET}"
    if ! bazel build --jobs=$HALF_CPUS ${PROTO_TARGET} \
        --output_groups=${output_groups} \
        --verbose_failures; then
        log 31 ERROR "Proto构建失败"
        exit 1
    fi
    
    log 32 INFO "Proto构建成功！生成文件: \033[34m${pb_h_file}\033[0m"
    # 运行后会生成以下文件（路径在输出中显示）：
    # ```bash
    # bazel-bin/external/envoy_api/envoy/extensions/filters/http/hyperscan_scanner/v3/
    #   ├── pkg-descriptor-set.proto.bin  # 二进制描述符（用于动态反射）  该文件允许Envoy在运行时解析YAML配置
    # ```
    # 生成的cc和h文件在这个路径
    # zhouda@DESKTOP-UBOB8AD:~$ find . -type f -name "*.pb.h" | grep hy
    # ./.cache/bazel/_bazel_zhouda/c34b0efabd02733a23769c3dc34b2b77/execroot/envoy/bazel-out/k8-fastbuild/bin/external/envoy_api/envoy/extensions/filters/http/hyperscan_scanner/v3/hyperscan_scanner.pb.h
    #   ├── hyperscan_scanner.pb.h        # C++头文件（proto消息定义）   
    #   └── hyperscan_scanner.pb.cc       # C++实现（序列化/反序列化代码）
}

# 构建过滤器
build_filter() {
    log 32 INFO "开始构建过滤器..."
    log 33 NOTE "编译选项：C++20标准，禁用missing-requires警告"

    # ​​再编译过滤器实现 仅编译您的过滤器（不编译整个Envoy）
    if ! bazel build --jobs=$HALF_CPUS ${FILTER_TARGET} \
                --disk_cache=${BAZEL_CACHE_DIR} \
                --verbose_failures \
                --cxxopt="-Wno-error=missing-requires" \
                --cxxopt="-std=c++20" \
                --compilation_mode="fastbuild"; then
        log 31 ERROR "过滤器构建失败"
        exit 1
    fi

    # 查看生成的文件
    ls bazel-bin/source/extensions/filters/http/hyperscan_scanner/
    log 32 INFO "过滤器构建成功！"
}

# 构建Envoy
build_envoy() {
    # 默认情况下，Bazel 会将生成的可执行文件放在 bazel-bin 目录中（位于 Envoy 源码根目录下）。
    # ​​具体路径​​: bazel-bin/source/exe/envoy-static

    # --disk_cache 参数仅影响 ​​构建结果缓存​​（如编译生成的 .o 文件、二进制文件等），而不是外部依赖的存储位置。
    # --verbose_failures: 启用详细日志
    # -Wno-error=dangling-pointer: 阻止将悬垂指针警告视为错误。
    # -Wno-dangling-pointer: 完全禁用悬垂指针警告。
    # 确保选项应用到所有依赖项​​ 如果V8作为外部依赖编译，可能需要通过--host_cxxopt传递选项
    # ​默认构建类型​​: Bazel 默认使用 fastbuild 模式（包含调试符号，未优化）。
    # ​​优化构建​​: 若需要发布优化版本，可添加 -c opt 参数: 会增加编译时间
    
    log 32 INFO "开始构建Envoy..."
    log 33 NOTE "编译选项：禁用tcmalloc"
    
    local build_flags=(
        "--disk_cache=${BAZEL_CACHE_DIR}"
        "--verbose_failures"
        "--define=tcmalloc=disabled"
        "--compilation_mode=fastbuild"
        "--copt=-ggdb"
    )
    
    if ! bazel build --jobs=$HALF_CPUS -c dbg ${ENVOY_TARGET} "${build_flags[@]}"; then
        log 31 ERROR "Envoy构建失败"
        exit 1
    fi

    local envoy_binary="bazel-bin/source/exe/envoy-static"
    if [ ! -f "${envoy_binary}" ]; then
        log 31 ERROR "未找到Envoy可执行文件"
        exit 1
    fi
    log 32 INFO "Envoy构建成功！可执行文件: \033[34m${envoy_binary}\033[0m"
}



# 显示帮助信息
show_help() {
    echo -e "\033[36m用法: $0 [选项]\033[0m"
    echo -e "\033[36m选项:\033[0m"
    echo "  --proto     仅构建Proto定义"
    echo "  --filter    仅构建过滤器"
    echo "  --envoy     仅构建Envoy"
    echo "  --all       构建全部组件（默认）"
    echo "  --clean     清理构建环境"
    echo -e "  --help      显示帮助信息 \033[33m(当前页面)\033[0m"
}


# 主函数
main() {
    local build_proto_flag=0
    local build_filter_flag=0
    local build_envoy_flag=0
    local clean_flag=0

    # 解析参数
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --proto)
                build_proto_flag=1
                shift
                ;;
            --filter)
                build_filter_flag=1
                shift
                ;;
            --envoy)
                build_envoy_flag=1
                shift
                ;;
            --all)
                build_proto_flag=1
                build_filter_flag=1
                build_envoy_flag=1
                shift
                ;;
            --clean)
                clean_flag=1
                shift
                ;;
            --help)
                show_help
                exit 0
                ;;
            *)
                log 31 ERROR "未知参数 $1"
                show_help
                exit 1
                ;;
        esac
    done

    # 默认构建全部
    if [ ${build_proto_flag} -eq 0 ] && [ ${build_filter_flag} -eq 0 ] && [ ${build_envoy_flag} -eq 0 ]; then
        if [ ${clean_flag} -eq 0 ]; then
            build_proto_flag=1
            build_filter_flag=1
            build_envoy_flag=1
        fi
    fi

    # 执行清理
    if [ ${clean_flag} -eq 1 ]; then
        clean_build
    fi

    # 执行构建
    if [ ${build_proto_flag} -eq 1 ] || [ ${build_filter_flag} -eq 1 ] || [ ${build_envoy_flag} -eq 1 ]; then
        prepare_build
    fi

    [ ${build_proto_flag} -eq 1 ] && build_proto
    [ ${build_filter_flag} -eq 1 ] && build_filter
    [ ${build_envoy_flag} -eq 1 ] && build_envoy

    log 32 INFO "构建流程完成"
}

# 启动主函数
main "$@"