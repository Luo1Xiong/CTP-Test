#!/bin/bash

################################################################################
# CTP交易API测试程序 - 从配置文件运行
################################################################################

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# 脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG_FILE="${SCRIPT_DIR}/config.json"
BUILD_DIR="${SCRIPT_DIR}/build"

# 打印函数
print_info()    { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_error()   { echo -e "${RED}[ERROR]${NC} $1"; }
print_header()  { echo -e "${CYAN}====================================${NC}"; echo -e "${CYAN}$1${NC}"; echo -e "${CYAN}====================================${NC}"; }

# 从JSON文件读取配置（不依赖jq）
read_config() {
    local key="$1"
    # 使用grep和sed提取JSON值
    grep -o "\"$key\"[[:space:]]*:[[:space:]]*\"[^\"]*\"" "$CONFIG_FILE" | \
        sed 's/.*: *"\(.*\)".*/\1/' || echo ""
}

# 检查配置文件
check_config() {
    if [ ! -f "$CONFIG_FILE" ]; then
        print_error "配置文件不存在: $CONFIG_FILE"
        exit 1
    fi
}

# 读取配置
print_header "读取配置"
check_config

TD_HOST=$(read_config "tdHost")
BROKER_ID=$(read_config "brokerId")
INVESTOR_ID=$(read_config "investorId")
PASSWORD=$(read_config "password")
APP_ID=$(read_config "appId")
AUTH_CODE=$(read_config "authCode")

# 验证必需字段
if [ -z "$TD_HOST" ] || [ -z "$BROKER_ID" ] || [ -z "$INVESTOR_ID" ] || [ -z "$PASSWORD" ]; then
    print_error "配置文件缺少必需字段"
    exit 1
fi

# 显示配置信息
echo ""
print_info "交易服务器: $TD_HOST"
print_info "经纪公司:    $BROKER_ID"
print_info "投资者:      $INVESTOR_ID"
print_info "应用ID:      $APP_ID"
echo ""

# 检查是否已编译
if [ ! -f "$BUILD_DIR/ctp_trader_test" ]; then
    print_info "程序未编译，开始编译..."
    bash "$SCRIPT_DIR/build.sh" -b
fi

# 运行程序
print_header "启动CTP交易API测试"
echo ""

cd "$BUILD_DIR"
export LD_LIBRARY_PATH="${SCRIPT_DIR}/../Framework/Linux:$LD_LIBRARY_PATH"

./ctp_trader_test \
    -f "$TD_HOST" \
    -b "$BROKER_ID" \
    -u "$INVESTOR_ID" \
    -p "$PASSWORD" \
    -i "$INVESTOR_ID" \
    -a "$APP_ID" \
    -c "$AUTH_CODE"

echo ""
print_header "程序退出"
