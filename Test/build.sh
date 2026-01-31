#!/bin/bash

################################################################################
# CTP交易API测试程序 - 编译并运行脚本
################################################################################

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
CTP_LIB_DIR="${SCRIPT_DIR}/../Framework/Linux"
BUILD_LOG="${SCRIPT_DIR}/build_error.log"

# 打印带颜色的消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_header() {
    echo ""
    echo -e "${BLUE}====================================${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}====================================${NC}"
}

# 检查依赖
check_dependencies() {
    print_header "检查依赖"

    if ! command -v cmake &> /dev/null; then
        print_error "未找到 cmake，请先安装: sudo apt-get install cmake"
        exit 1
    fi
    print_success "cmake 已安装"

    if ! command -v make &> /dev/null; then
        print_error "未找到 make，请先安装: sudo apt-get install build-essential"
        exit 1
    fi
    print_success "make 已安装"

    if [ ! -d "${CTP_LIB_DIR}" ]; then
        print_error "未找到CTP库目录: ${CTP_LIB_DIR}"
        exit 1
    fi
    print_success "CTP库目录存在"
}

# 清理构建目录
clean_build() {
    print_info "清理构建目录..."
    if [ -d "${BUILD_DIR}" ]; then
        rm -rf "${BUILD_DIR}"
        print_success "构建目录已清理"
    fi
}

# 编译项目
build_project() {
    print_header "编译项目"

    # 清空日志文件
    > "${BUILD_LOG}"
    print_info "编译日志: ${BUILD_LOG}"

    # 自动清理构建目录
    clean_build

    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"

    print_info "配置 CMake..."
    cmake .. -DCMAKE_BUILD_TYPE=Release

    print_info "编译..."
    # 保存到临时文件，同时显示到终端
    local tmp_log="${BUILD_DIR}/make_output.log"
    if ! make -j$(nproc) 2>&1 | tee "${tmp_log}"; then
        print_error "编译失败，错误已保存到: ${BUILD_LOG}"
    fi
    # 只提取错误到最终日志（包括 undefined reference, ld returned 等）
    grep -iE "error:|undefined reference|ld returned" "${tmp_log}" > "${BUILD_LOG}" || true
    rm -f "${tmp_log}"

    print_success "编译完成!"
}

# 运行程序
run_program() {
    print_header "运行程序"

    cd "${BUILD_DIR}"

    # 检查可执行文件
    if [ ! -f "./ctp_trader_test" ]; then
        print_error "未找到可执行文件: ctp_trader_test"
        exit 1
    fi

    # 复制配置文件到build目录
    if [ -f "${SCRIPT_DIR}/config.json" ]; then
        cp "${SCRIPT_DIR}/config.json" "${BUILD_DIR}/config.json"
        print_info "已加载配置文件: config.json"
    fi

    # 设置库路径
    export LD_LIBRARY_PATH="${CTP_LIB_DIR}:$LD_LIBRARY_PATH"

    print_info "库路径: ${CTP_LIB_DIR}"
    echo ""

    # 运行程序
    mkdir -p flow
    ./ctp_trader_test "$@"
}

# 显示使用帮助
show_help() {
    cat << EOF
用法: $0 [选项] [程序参数]

选项:
    -c, --clean       清理构建目录后重新编译
    -b, --build       仅编译，不运行
    -r, --run         仅运行，不编译（假设已编译）
    -h, --help        显示此帮助信息

程序参数 (会传递给 ctp_trader_test):
    -f <地址>         交易服务器地址 (格式: tcp://ip:port)
    -b <经纪商>       经纪公司代码
    -u <用户名>       用户名
    -p <密码>         密码
    -i <投资者>       投资者代码 (默认与用户名相同)
    -a <AppID>        应用ID (用于认证)
    -c <AuthCode>     认证码

示例:
    # 直接编译并运行（使用交互式输入参数）
    $0

    # 清理后编译并运行（使用默认SimNow环境）
    $0 -c -f tcp://180.168.146.187:10130 -b 9999 -u 用户名 -p 密码

    # 仅编译
    $0 -b

    # 仅运行（已编译过）
    $0 -r -f tcp://180.168.146.187:10130 -b 9999 -u 用户名 -p 密码

SimNow测试环境:
    电信: tcp://180.168.146.187:10130
    移动: tcp://180.168.146.187:10130
    经纪公司: 9999

EOF
}

# 主函数
main() {
    # 解析脚本参数
    CLEAN=false
    BUILD_ONLY=false
    RUN_ONLY=false
    PROGRAM_ARGS=()

    while [[ $# -gt 0 ]]; do
        case $1 in
            -c|--clean)
                CLEAN=true
                shift
                ;;
            -b|--build)
                BUILD_ONLY=true
                shift
                ;;
            -r|--run)
                RUN_ONLY=true
                shift
                ;;
            -h|--help)
                show_help
                exit 0
                ;;
            *)
                # 其他参数传递给程序
                PROGRAM_ARGS+=("$1")
                shift
                ;;
        esac
    done

    print_header "CTP交易API测试程序"

    # 如果不是仅运行模式，则检查依赖并编译
    if [ "$RUN_ONLY" = false ]; then
        check_dependencies

        if [ "$CLEAN" = true ]; then
            clean_build
        fi

        build_project

        # 如果是仅编译模式，编译完成后退出
        if [ "$BUILD_ONLY" = true ]; then
            print_success "编译完成，程序位于: ${BUILD_DIR}/ctp_trader_test"
            exit 0
        fi
    fi

    # 运行程序
    run_program "${PROGRAM_ARGS[@]}"
}

# 捕获Ctrl+C信号
trap 'echo ""; print_warning "收到中断信号，退出..."; exit 130' INT

# 执行主函数
main "$@"
