# OpenHarmony 4.0 Beta2 / RK3568 接入

## 1. 放置模块

将本仓库克隆到 OpenHarmony 源码根目录，并确保目录名为 `pqc_mlkem`：

```sh
cd /path/to/OpenHarmony
git clone https://github.com/Hayd-coder/openharmony-pqc-mlkem.git pqc_mlkem
```

## 2. 注册子系统

在 `build/subsystem_config.json` 最外层对象的末尾加入以下成员。注意给前一个成员补逗号，
并保持整个文件仍是合法 JSON：

```json
"pqc_mlkem": {
  "path": "pqc_mlkem",
  "name": "pqc_mlkem"
}
```

## 3. 加入 RK3568 产品

在 `vendor/hihope/rk3568/config.json` 的 `subsystems` 数组末尾加入：

```json
{
  "subsystem": "pqc_mlkem",
  "components": [
    {
      "component": "pqc_mlkem",
      "features": []
    }
  ]
}
```

同样要确认前一个数组元素后有逗号，且文件仍是合法 JSON。

## 4. 编译

```sh
./build.sh --product-name rk3568 --build-target pqc_mlkem
```

主要产物包括 HAP、基线测试程序和 NEON 测试程序。HAP 的固定输出位置为：

```text
out/rk3568/obj/pqc_mlkem/PqcMlkemDemo.hap
```

不同 OpenHarmony 构建版本可能调整可执行文件的具体安装路径，可在 `out/rk3568/` 下按文件名查找。

## 5. HDC 安装与启动

```sh
HDC=out/sdk/ohos-sdk/linux/toolchains/hdc
$HDC list targets
$HDC install -r out/rk3568/obj/pqc_mlkem/PqcMlkemDemo.hap
$HDC shell aa start -a EntryAbility -b com.example.pqcmlkem
```

如果 `list targets` 显示 `[Empty]`，确认数据线接在 DAYU200 的 USB 3.0 OTG 口，并在完整断电
重启板卡后再次检查。
