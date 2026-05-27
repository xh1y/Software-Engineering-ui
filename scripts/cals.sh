#!/bin/bash
# cals.sh — 计算 src/ 目录的 语句 + 分支 覆盖率总计

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

gcovr -r "$PROJECT_DIR" \
      --filter "$PROJECT_DIR/src/" \
      --exclude ".*_autogen.*" \
      --branches \
      --exclude-unreachable-branches \
      2>&1 | grep "^src/" | awk '{
    # gcovr --branches 输出格式:
    # FILE   LINES_EXEC  LINES_TOTAL  LINE%  BRANCHES_EXEC  BRANCHES_TOTAL  BRANCH%
    # $1     $2          $3           $4     $5             $6              $7

    lines_total   += $2
    lines_exec    += $3
    branch_exec   += $5
    branch_total  += $6
    count++
  } END {
    if (count == 0) {
      printf "No source files found.\n"
      exit 1
    }
    printf "Total files:     %d\n", count
    printf "Total lines:     %d\n", lines_total
    printf "Lines executed:  %d\n", lines_exec
    printf "Line coverage:   %.1f%%\n", (lines_total > 0 ? (lines_exec / lines_total) * 100 : 0)
    printf "Total branches:  %d\n", branch_total
    printf "Branches taken:  %d\n", branch_exec
    printf "Branch coverage: %.1f%%\n", (branch_total > 0 ? (branch_exec / branch_total) * 100 : 0)
  }'
