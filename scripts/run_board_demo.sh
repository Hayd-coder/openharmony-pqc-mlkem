#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "用法：HDC_BIN=/path/to/hdc $0 PqcMlkemDemo-rk3568-v1.0.2.hap" >&2
  exit 2
fi

hap_path=$1
hdc_bin=${HDC_BIN:-hdc}

if [[ ! -f "$hap_path" ]]; then
  echo "[FAIL] 找不到 HAP：$hap_path" >&2
  exit 1
fi

if [[ "$hdc_bin" == */* ]]; then
  if [[ ! -x "$hdc_bin" ]]; then
    echo "[FAIL] HDC 不可执行：$hdc_bin" >&2
    exit 1
  fi
elif ! command -v "$hdc_bin" >/dev/null 2>&1; then
  echo "[FAIL] 找不到 hdc。请设置 HDC_BIN=/path/to/hdc" >&2
  exit 1
fi

targets=$($hdc_bin list targets 2>&1)
echo "$targets"
if [[ -z "$targets" || "$targets" == *"[Empty]"* ]]; then
  echo "[FAIL] 没有发现 HDC 设备。请检查板卡电源和 USB 3.0 OTG 数据线。" >&2
  exit 1
fi

$hdc_bin install -r "$hap_path"
$hdc_bin shell aa force-stop com.example.pqcmlkem >/dev/null 2>&1 || true
$hdc_bin shell aa start -a EntryAbility -b com.example.pqcmlkem

echo "[PASS] HAP 已安装，EntryAbility 已启动"
