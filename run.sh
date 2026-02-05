#!/bin/bash

# ==============================================================================
#  STM32 Build & Flash Utility (Ultimate Fixed Edition)
#  Updates: Fixed Linker Script Conflict, Enhanced Stability
# ==============================================================================

# --- ⚙️ 系统级配置 ---
set -o pipefail
export CMAKE_COLOR_DIAGNOSTICS=ON
export GCC_COLORS='error=01;31:warning=01;35:note=01;36:caret=01;32:locus=01:quote=01'

# --- 🎨 样式定义 ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
GREY='\033[0;90m'
NC='\033[0m'

BOLD='\033[1m'
BG_RED='\033[41m'
BG_GREEN='\033[42m'
BG_YELLOW='\033[43m'
BG_BLUE='\033[44m'

# --- ⚙️ 用户配置区 ---
DEFAULT_SERIES="f4"
INTERFACE_CFG="interface/cmsis-dap.cfg" # 调试器配置

declare -A CHIP_MAP
CHIP_MAP=( 
    ["f1"]="stm32f1x" 
    ["f4"]="stm32f4x" 
    ["f7"]="stm32f7x" 
    ["g4"]="stm32g4x" 
    ["h7"]="stm32h7x" 
    ["l4"]="stm32l4x" 
)

CHIP_ARG=${1:-$DEFAULT_SERIES}
TARGET_CFG=${CHIP_MAP[$CHIP_ARG]}
CLEAN_MODE=$2

# --- 🔍 目录与文件识别 ---
SCRIPT_DIR=$(dirname "$(realpath "$0")")
PROJECT_DIR=$SCRIPT_DIR
while [ ! -f "$PROJECT_DIR/CMakeLists.txt" ]; do
    PROJECT_DIR=$(dirname "$PROJECT_DIR")
    if [ "$PROJECT_DIR" == "/" ]; then echo "Error: CMakeLists.txt not found."; exit 1; fi
done
PROJECT_NAME=$(basename "$PROJECT_DIR")
BUILD_DIR="$PROJECT_DIR/build"
# 查找 .ld 文件 (仅用于资源大小分析，不强制传给链接器)
LD_FILE=$(find "$PROJECT_DIR" -maxdepth 1 -name "*.ld" | head -n 1)

# --- ℹ️ 信息采集 ---
if git -C "$PROJECT_DIR" rev-parse --git-dir > /dev/null 2>&1; then
    GIT_HASH=$(git -C "$PROJECT_DIR" rev-parse --short HEAD)
    GIT_BRANCH=$(git -C "$PROJECT_DIR" rev-parse --abbrev-ref HEAD)
    GIT_INFO="${GIT_BRANCH} @ ${GIT_HASH}"
else
    GIT_INFO="Non-Git Project"
fi
GCC_VER=$(arm-none-eabi-gcc -dumpversion 2>/dev/null || echo "Unknown")
BUILD_TIME=$(date "+%Y-%m-%d %H:%M:%S")

# --- 📟 核心功能函数 ---
print_line() { echo -e "${GREY}------------------------------------------------------------${NC}"; }
log_step() { echo -e "\n${CYAN}[$1/$2] ${BOLD}$3${NC}"; print_line; }

indent_output() {
    while IFS= read -r line; do
        echo -e "   ${GREY}│${NC} $line"
    done
}

panic_handler() {
    local step_name=$1
    echo -e ""
    echo -e "${BG_RED}${WHITE}${BOLD}  FATAL ERROR  ${NC} ${RED}Process failed at: $step_name${NC}"
    echo -e "${GREY}┌──────────────────────────────────────────────────────────┐${NC}"
    echo -e "${GREY}│${NC} ${RED}Possible Causes:${NC}                                         ${GREY}│${NC}"
    echo -e "${GREY}│${NC}  1. Compilation syntax error (check logs above).         ${GREY}│${NC}"
    echo -e "${GREY}│${NC}  2. Missing dependencies or wrong toolchain version.     ${GREY}│${NC}"
    echo -e "${GREY}│${NC}  3. Linker script (.ld) memory overflow.                 ${GREY}│${NC}"
    echo -e "${GREY}│${NC}  4. Debugger connection lost (if flashing).              ${GREY}│${NC}"
    echo -e "${GREY}└──────────────────────────────────────────────────────────┘${NC}"
    exit 1
}

draw_bar() {
    local used=$1; local total=$2; local color=$3; local width=30
    if [ "$total" -eq 0 ]; then total=1; fi
    local percent=$(awk "BEGIN {printf \"%.0f\", ($used/$total)*100}")
    local fill_len=$(awk "BEGIN {printf \"%.0f\", ($used/$total)*$width}")
    if [ "$fill_len" -gt "$width" ]; then fill_len=$width; fi
    local bar=""; for ((i=0; i<width; i++)); do if [ $i -lt $fill_len ]; then bar="${bar}#"; else bar="${bar}."; fi; done
    printf "${color}[%-s] ${BOLD}%3s%%${NC}" "$bar" "$percent"
}

# --- 🏁 仪表盘显示 ---
clear
print_line
echo -e "${BOLD}${WHITE}  STM32 AUTOMATED BUILD SYSTEM ${NC}"
print_line
printf "${GREY}%-14s : ${CYAN}%s${NC}\n" "Project"   "$PROJECT_NAME"
printf "${GREY}%-14s : ${WHITE}%s${NC}\n" "Timestamp" "$BUILD_TIME"
printf "${GREY}%-14s : ${YELLOW}%s${NC}\n" "Git Version" "$GIT_INFO"
echo ""
printf "${GREY}%-14s : ${WHITE}%s${NC}\n" "Target Chip" "${CHIP_ARG^^} (${TARGET_CFG})"
printf "${GREY}%-14s : ${NC}%s${NC}\n" "GCC Version" "v$GCC_VER"
print_line

# --- 🛠️ [Step 1] 环境准备 ---
log_step 1 4 "Environment Preparation"
BUILD_TYPE="Incremental Build"

if [ "$CLEAN_MODE" == "clean" ]; then
    echo " >> [CLEAN MODE] Removing build directory..." | indent_output
    rm -rf "$BUILD_DIR"
    BUILD_TYPE="Clean Build (Rebuild All)"
fi

if [ ! -d "$BUILD_DIR" ]; then
    echo " >> Creating build directory..." | indent_output
    mkdir -p "$BUILD_DIR"
fi
echo " >> Mode: $BUILD_TYPE" | indent_output

# --- 🛠️ [Step 2] 编译 (修复版) ---
log_step 2 4 "Compiling Source Code"

# 1. 配置 CMake
# 注意：这里不再强制添加 -T 链接脚本参数，避免与 CMakeLists.txt 冲突
echo " >> Configuring CMake (Ninja)..." | indent_output
cmake -B "$BUILD_DIR" -S "$PROJECT_DIR" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="./cmake/gcc-arm-none-eabi.cmake" \
    -DCMAKE_BUILD_TYPE="Debug" \
    2>&1 | indent_output

if [ $? -ne 0 ]; then panic_handler "CMake Configuration"; fi

# 2. 执行编译
echo " >> Building target..." | indent_output
cmake --build "$BUILD_DIR" -j12 2>&1 | indent_output

if [ $? -ne 0 ]; then panic_handler "Compilation / Linking"; fi

# 3. 生成二进制文件
ELF_FILE="$BUILD_DIR/${PROJECT_NAME}.elf"
if [ ! -f "$ELF_FILE" ]; then panic_handler "ELF Generation"; fi

echo " >> Generating binaries..." | indent_output
arm-none-eabi-objcopy -O ihex "$ELF_FILE" "$BUILD_DIR/${PROJECT_NAME}.hex"
arm-none-eabi-objcopy -O binary "$ELF_FILE" "$BUILD_DIR/${PROJECT_NAME}.bin"

echo -e "\n${BG_GREEN}${WHITE}${BOLD}  SUCCESS  ${NC} ${GREEN}Build complete. Firmware generated.${NC}"

# --- 📊 [Step 3] 资源分析 ---
log_step 3 4 "Resource Usage Analysis"

# 检查 LD 文件是否存在，如果不存在则给出警告但不中止脚本
if [ -z "$LD_FILE" ]; then
    echo -e " ${YELLOW}>> Warning: .ld file not found. Skipping resource analysis.${NC}" | indent_output
else
    extract_size() {
        local key=$1; local val=$(grep -E "$key.*LENGTH =" "$LD_FILE" | grep -oE "[0-9]+[KMG]" | head -n 1)
        local num=$(echo "$val" | grep -oE "[0-9]+"); local unit=$(echo "$val" | grep -oE "[KMG]")
        if [ "$unit" == "K" ]; then echo $((num * 1024)); elif [ "$unit" == "M" ]; then echo $((num * 1024 * 1024)); else echo $num; fi
    }
    
    TOTAL_FLASH=$(extract_size "FLASH")
    TOTAL_RAM=$(extract_size "RAM")
    
    if [ -z "$TOTAL_FLASH" ] || [ -z "$TOTAL_RAM" ]; then
        echo -e " ${YELLOW}>> Warning: Could not parse memory size from .ld file.${NC}" | indent_output
    else
        READING=$(arm-none-eabi-size "$ELF_FILE" | tail -n 1)
        TEXT=$(echo "$READING" | awk '{print $1}')
        DATA=$(echo "$READING" | awk '{print $2}')
        BSS=$(echo "$READING" | awk '{print $3}')
        
        USED_FLASH=$((TEXT + DATA))
        USED_RAM=$((DATA + BSS))

        printf "${BOLD}%-6s %-12s %-12s %-40s${NC}\n" "TYPE" "USED(B)" "TOTAL(B)" "UTILIZATION"
        echo -e "${GREY}------ ------------ ------------ ----------------------------------------${NC}"
        printf "${CYAN}%-6s${NC} %-12d %-12d " "FLASH" "$USED_FLASH" "$TOTAL_FLASH"; draw_bar $USED_FLASH $TOTAL_FLASH $CYAN; echo ""
        printf "${CYAN}%-6s${NC} %-12d %-12d " "RAM" "$USED_RAM" "$TOTAL_RAM"; draw_bar $USED_RAM $TOTAL_RAM $YELLOW; echo ""
        echo ""
    fi
fi

# --- 💾 [Step 4] 智能烧录 (静默检测) ---
log_step 4 4 "Smart Flashing"
WIN_HEX=$(cygpath -m "$BUILD_DIR/${PROJECT_NAME}.hex")

if [ -z "$TARGET_CFG" ]; then 
    echo -e " ${YELLOW}>> Warning: Unknown chip series '$CHIP_ARG'. Skipping flash.${NC}" | indent_output
    exit 0
fi

PROBE_CMD_ARGS=""
if [[ "$INTERFACE_CFG" == *"stlink"* ]]; then
    PROBE_CMD_ARGS="transport select hla_swd"
else
    PROBE_CMD_ARGS="transport select swd"
fi

echo " >> Detecting Probe ($INTERFACE_CFG)..." | indent_output

openocd -f "$INTERFACE_CFG" \
        -c "adapter speed 4000; $PROBE_CMD_ARGS; init; shutdown" \
        > /dev/null 2>&1

PROBE_STATUS=$?

if [ $PROBE_STATUS -ne 0 ]; then
    echo -e "\n${BG_BLUE}${WHITE}${BOLD}  SKIPPED  ${NC} ${BLUE}No debugger detected. Build only mode.${NC}\n"
    exit 0
fi

echo " >> Probe detected. Programming target..." | indent_output
openocd -f "$INTERFACE_CFG" \
        -f target/${TARGET_CFG}.cfg \
        -c "adapter speed 4000; $PROBE_CMD_ARGS; program \"$WIN_HEX\" verify reset exit" \
        2>&1 | indent_output

if [ $? -eq 0 ]; then
    echo -e "\n${BG_GREEN}${WHITE}${BOLD}  SUCCESS  ${NC} ${GREEN}Target Flashed & Reset.${NC}\n"
else
    panic_handler "Flashing / Verification"
fi