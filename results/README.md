# DAYU200 实验记录

测试平台为润和 HH-SCDAYU200，处理器为 RK3568，系统为 OpenHarmony 4.0 Beta2，测试参数集
为 ML-KEM-512。

## 性能数据

- `dayu200_mlkem512_baseline_20261021.csv`：早期可移植 C 基线，每项执行 1000 次；
- `dayu200_mlkem512_comparison_20261021.csv`：正式对比原始数据，5 轮、每项每轮 10000 次；
- `dayu200_mlkem512_summary_20261021.csv`：正式数据的均值、标准差、耗时降低和加速比。

正式测试在同一块板卡、同一系统和相同参数下执行。可移植 C 与 AArch64/NEON 两种实现
交替运行，测试前均通过正确性和篡改检查。

## 最终版界面截图

以下截图于 2026-10-23 从 DAYU200 实机抓取，使用的是支持手动输入消息的最终版本：

- `dayu200_manual_input_20261023.jpeg`：可编辑消息输入框；
- `dayu200_manual_encrypted_20261023.jpeg`：生成密钥并加密后的结果；
- `dayu200_manual_decrypted_20261023.jpeg`：解密明文及一致性、完整性校验；
- `dayu200_manual_ciphertext_20261023.jpeg`：消息密文、KEM 密文、Nonce 和认证标签；
- `dayu200_manual_comparison_20261023.jpeg`：可移植 C 与 AArch64/NEON 的正式对比数据。
