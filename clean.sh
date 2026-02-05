#!/bin/bash

# ==============================================================================
#  STM32 Artifact Cleaner (Professional Edition)
#  Keeps: .elf, .bin, .hex | Removes: Everything else
# ==============================================================================

# --- ⚙️ 系统级配置 ---
set -o pipefail

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
BG_BLUE='\033[44m'

# --- 🔍 目录识别 ---
SCRIPT_DIR=$(dirname "$(realpath "$0")")
PROJECT_DIR=$SCRIPT_DIR
while [ ! -f "$PROJECT_DIR/CMakeLists.txt" ]; do
    PROJECT_DIR=$(dirname "$PROJECT_DIR")
    if [ "$PROJECT_DIR" == "/" ]; then echo "Error: CMakeLists.txt not found."; exit 1; fi
done
PROJECT_NAME=$(basename "$PROJECT_DIR")
BUILD_DIR="$PROJECT_DIR/build"

# --- 📟 辅助函数 ---
print_line() { echo -e "${GREY}------------------------------------------------------------${NC}"; }
log_step() { echo -e "\n${CYAN}[$1/$2] ${BOLD}$3${NC}"; print_line; }

indent_output() {
    while IFS= read -r line; do
        echo -e "   ${GREY}│${NC} $line"
    done
}

# 获取目录占用大小 (单位 KB)
get_dir_size() {
    du -sk "$BUILD_DIR" | cut -f1
}

# 格式化大小显示 (KB -> MB/KB)
format_size() {
    local size_kb=$1
    if [ "$size_kb" -gt 1024 ]; then
        echo $(awk "BEGIN {printf \"%.2f MB\", $size_kb/1024}")
    else
        echo "${size_kb} KB"
    fi
}

# --- 🏁 仪表盘 ---
clear
print_line
echo -e "${BOLD}${WHITE}  STM32 BUILD CLEANER UTILITY ${NC}"
print_line
printf "${GREY}%-14s : ${CYAN}%s${NC}\n" "Project" "$PROJECT_NAME"
printf "${GREY}%-14s : ${WHITE}%s${NC}\n" "Target Dir" "build/"
printf "${GREY}%-14s : ${YELLOW}%s${NC}\n" "Policy" "Keep Firmware Only (.hex .bin .elf)"
print_line

if [ ! -d "$BUILD_DIR" ]; then
    echo -e "\n${BG_RED}${WHITE}${BOLD}  ERROR  ${NC} ${RED}Build directory not found.${NC}\n"
    exit 1
fi

# --- 📊 [Step 1] 扫描 ---
log_step 1 3 "Scanning Directory"

INITIAL_SIZE=$(get_dir_size)
FILE_COUNT=$(find "$BUILD_DIR" -type f | wc -l)

echo " >> Current usage: $(format_size $INITIAL_SIZE)" | indent_output
echo " >> Total files found: $FILE_COUNT" | indent_output

# 检查是否有需要保留的文件，如果没有，发出警告
KEEP_COUNT=$(find "$BUILD_DIR" -maxdepth 1 \( -name "*.hex" -o -name "*.bin" -o -name "*.elf" \) | wc -l)

if [ "$KEEP_COUNT" -eq 0 ]; then
    echo -e " >> ${YELLOW}Warning: No firmware files (.bin/.hex/.elf) found!${NC}" | indent_output
    echo -e " >> ${YELLOW}Directory will be completely emptied.${NC}" | indent_output
else
    echo " >> Firmware artifacts detected: $KEEP_COUNT" | indent_output
fi

# --- 🧹 [Step 2] 执行清理 ---
log_step 2 3 "Purging Junk Files"

# 进入目录
cd "$BUILD_DIR" || exit

# 逻辑：查找所有文件和文件夹（除了 . .. 和 指定后缀），然后删除
# 使用 -v 显示被删除的文件（可选，这里为了简洁只统计数量）
# find . -maxdepth 1 ! -name "." ! -name "*.elf" ! -name "*.bin" ! -name "*.hex" -print

echo " >> Removing object files, cache, and ninja logs..." | indent_output

# 核心清理命令
# 1. 删除所有子目录 (CMakeCache, CMakeFiles 等)
find . -mindepth 1 -maxdepth 1 -type d -exec rm -rf {} +

# 2. 删除当前目录下所有非白名单文件
find . -maxdepth 1 -type f \
    ! -name "*.elf" \
    ! -name "*.bin" \
    ! -name "*.hex" \
    -delete

echo " >> Cleanup operation completed." | indent_output

# --- 📈 [Step 3] 结果报告 ---
log_step 3 3 "Result Analysis"

FINAL_SIZE=$(get_dir_size)
SAVED_SIZE=$((INITIAL_SIZE - FINAL_SIZE))
FINAL_COUNT=$(find . -maxdepth 1 -type f | wc -l)

# 简单的 ASCII 进度条模拟
draw_bar() {
    local percent=$1
    local width=30
    local fill_len=$(awk "BEGIN {printf \"%.0f\", ($percent/100)*$width}")
    local bar=""; for ((i=0; i<width; i++)); do if [ $i -lt $fill_len ]; then bar="${bar}#"; else bar="${bar}."; fi; done
    echo "$bar"
}

# 计算减少百分比
if [ "$INITIAL_SIZE" -gt 0 ]; then
    REDUCTION_PERC=$(awk "BEGIN {printf \"%.0f\", ($SAVED_SIZE/$INITIAL_SIZE)*100}")
else
    REDUCTION_PERC=0
fi

printf "${BOLD}%-15s %-15s %-15s${NC}\n" "METRIC" "BEFORE" "AFTER"
echo -e "${GREY}--------------- --------------- ---------------${NC}"
printf "${CYAN}%-15s${NC} %-15s %-15s\n" "Size" "$(format_size $INITIAL_SIZE)" "$(format_size $FINAL_SIZE)"
printf "${CYAN}%-15s${NC} %-15s %-15s\n" "Files" "$FILE_COUNT" "$FINAL_COUNT"
echo ""

echo -e " >> Reclaimed Space : ${GREEN}$(format_size $SAVED_SIZE)${NC}"
echo -e " >> Efficiency      : ${YELLOW}[$(draw_bar $REDUCTION_PERC)] ${REDUCTION_PERC}%${NC}"

echo -e "\n${BG_GREEN}${WHITE}${BOLD}  CLEANED  ${NC} ${GREEN}Build folder optimized.${NC}\n"