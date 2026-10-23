# DAYU200 实验数据

测试日期：2026-10-21

平台：润和 DAYU200，Rockchip RK3568，AArch64，OpenHarmony 4.0 Beta2

算法参数：ML-KEM-512

## CSV 文件

- `dayu200_mlkem512_baseline_20261021.csv`：早期可移植 C 基线，每项 1000 次。
- `dayu200_mlkem512_comparison_20261021.csv`：正式对比原始数据，5 轮、每项每轮 10000 次。
- `dayu200_mlkem512_summary_20261021.csv`：正式数据的均值、标准差、耗时降低和加速比。

正式测试在同一块板卡、同一系统环境中执行，并交替可移植 C 与 AArch64/NEON 的运行顺序，
以降低温度、系统状态和固定先后顺序带来的偏差。性能测试前，两种实现均通过正确性与篡改测试。

## 截图

- `dayu200_pqc_ui_final_before_20261021.jpeg`：加密前的输入界面。
- `dayu200_pqc_ui_final_after_20261021.jpeg`：加密、解密和校验后的界面。
- `dayu200_pqc_ui_final_metrics_20261021.jpeg`：板卡现场性能对比卡片。
