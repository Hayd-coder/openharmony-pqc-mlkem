#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

required_files=(
  BUILD.gn
  bundle.json
  Makefile
  LICENSE
  src/randombytes_ohos.c
  src/mlkem_selftest.c
  src/mlkem_benchmark.c
  src/mlkem_hybrid.h
  ui/native/pqc_mlkem_napi.c
  ui/entry/src/main/ets/pages/Index.ets
  ui/entry/src/main/module.json
  ui/AppScope/app.json
  ui/signature/pqc_mlkem_demo.p7b
  upstream/mlkem_native.c
  upstream/mlkem_native.h
  upstream/src/native/aarch64/src/mlkem_ntt_aarch64_asm.S
  docs/OPENHARMONY_INTEGRATION.md
  docs/DEMO_GUIDE.md
)

for relative_path in "${required_files[@]}"; do
  if [[ ! -f "$REPO_DIR/$relative_path" ]]; then
    echo "[FAIL] 缺少文件：$relative_path" >&2
    exit 1
  fi
done

echo "[PASS] 项目文件检查通过"
make -C "$REPO_DIR" test
make -C "$REPO_DIR" clean

if [[ $# -ge 1 ]]; then
  hap_path=$1
  if [[ ! -f "$hap_path" ]]; then
    echo "[FAIL] 找不到 HAP：$hap_path" >&2
    exit 1
  fi
  if ! command -v unzip >/dev/null 2>&1; then
    echo "[WARN] 未安装 unzip，跳过 HAP 内容检查"
  elif ! unzip -l "$hap_path" | grep -q 'libs/arm64-v8a/libpqc_mlkem_napi.so'; then
    echo "[FAIL] HAP 中缺少 ARM64 N-API 动态库" >&2
    exit 1
  else
    echo "[PASS] HAP 中包含 ARM64 N-API 动态库"
  fi
fi

echo "[PASS] 验证完成"
