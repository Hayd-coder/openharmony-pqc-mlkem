# OpenHarmony ML-KEM-512 实现与优化

面向润和 DAYU200（RK3568/AArch64）的毕业设计实验仓库。项目在 OpenHarmony 4.0 Beta2
上集成 ML-KEM-512，提供可移植 C 基线、AArch64/NEON 优化版本、正确性测试、性能测试，
以及可在板卡上现场输入消息并展示加密/解密结果的 ArkUI 应用。

## 项目范围与代码来源

本仓库只保留完成本课题所需的代码和实验材料，不包含原 OpenHarmony 源码树，也不包含
旧工程中的 SM9、Dilithium/ML-DSA 等学长项目。

- `src/`、`ui/`、`BUILD.gn`、`bundle.json`、`Makefile`：本课题的 OpenHarmony 适配、
  测试、性能测量、KEM-DEM 演示和可视化界面。
- `upstream/`：第三方密码核心
  [pq-code-package/mlkem-native](https://github.com/pq-code-package/mlkem-native)，固定于提交
  [`b9550181de754f798941b8a4f495f8a9b8b5e3ec`](https://github.com/pq-code-package/mlkem-native/commit/b9550181de754f798941b8a4f495f8a9b8b5e3ec)。
- `results/`：2026-10-21 在同一块 DAYU200 上获得的 CSV 原始数据、汇总和最终界面截图。

第三方代码保留了原文件中的版权声明和 SPDX 标识。详细信息见
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) 和
[`upstream/LICENSE.mlkem-native`](upstream/LICENSE.mlkem-native)。

## 功能

- ML-KEM-512 密钥生成、封装和解封装；
- 密钥、密文篡改及隐式拒绝测试；
- 可移植 C 与 AArch64/NEON 的同条件性能对比；
- ArkUI 手动输入消息、加密、密文展示、解密和完整性校验；
- 板卡现场重新测试并展示两种实现的耗时和加速效果。

ML-KEM 是密钥封装机制，不能直接加密任意长度消息。界面演示采用教学用 KEM-DEM：
ML-KEM 协商 32 字节共享密钥，SHAKE256 生成消息密钥流，SHA3-256 标签校验完整性。
该组合用于毕业设计实验，不应声明为经过安全认证的商用密码模块。

## 实验结果

DAYU200 上进行了 5 轮配对测试，每个操作每轮执行 10000 次，并交替两种实现的运行顺序。

| 操作 | 可移植 C（μs） | AArch64/NEON（μs） | 耗时降低 | 加速比 |
| --- | ---: | ---: | ---: | ---: |
| 密钥生成 | 79.003 | 58.313 | 26.19% | 1.355x |
| 密钥封装 | 91.054 | 65.254 | 28.34% | 1.395x |
| 密钥解封装 | 86.547 | 56.544 | 34.67% | 1.531x |

原始的 30 行测量记录和统计汇总分别位于：

- `results/dayu200_mlkem512_comparison_20261021.csv`
- `results/dayu200_mlkem512_summary_20261021.csv`

## 目录

```text
.
├── BUILD.gn                    OpenHarmony GN 构建规则
├── bundle.json                 OpenHarmony 部件描述
├── Makefile                    Linux 主机快速测试
├── src/                        适配、自测、基准与 KEM-DEM 逻辑
├── ui/                         ArkUI 页面、资源与 N-API 桥接
├── upstream/                   固定版本 mlkem-native 及 AArch64 后端
├── results/                    DAYU200 实测数据和截图
└── docs/                       OpenHarmony 接入说明
```

## 主机快速验证

Linux 主机需安装 C 编译器和 `make`：

```sh
make test
make benchmark
```

`make test` 会执行 ML-KEM 正反例测试，以及包含中文 UTF-8 明文的 KEM-DEM 加解密与
密文篡改拒绝测试。构建文件输出到被 Git 忽略的 `build/`。

## 集成到 OpenHarmony

在 OpenHarmony 4.0 Beta2 源码根目录执行：

```sh
git clone https://github.com/Hayd-coder/openharmony-pqc-mlkem.git pqc_mlkem
```

随后按 [`docs/OPENHARMONY_INTEGRATION.md`](docs/OPENHARMONY_INTEGRATION.md) 将部件加入
`build/subsystem_config.json` 和 `vendor/hihope/rk3568/config.json`，再编译：

```sh
./build.sh --product-name rk3568 --build-target pqc_mlkem
```

HAP 输出位置：

```text
out/rk3568/obj/pqc_mlkem/PqcMlkemDemo.hap
```

连接 DAYU200 的 USB 3.0 OTG 口后安装并启动：

```sh
HDC=out/sdk/ohos-sdk/linux/toolchains/hdc
$HDC install -r out/rk3568/obj/pqc_mlkem/PqcMlkemDemo.hap
$HDC shell aa start -a EntryAbility -b com.example.pqcmlkem
```

## 许可证

本课题新增代码采用 [Apache License 2.0](LICENSE)。`upstream/` 中的第三方文件仍适用
各文件标注的许可证；本仓库按 mlkem-native 提供的许可选项以 Apache-2.0 使用其核心代码。
