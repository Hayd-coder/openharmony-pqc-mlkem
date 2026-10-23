# DAYU200 现场演示说明

## 准备工作

1. 给 DAYU200 接通电源；
2. 使用数据线连接板卡的 USB 3.0 OTG 口，不要接到普通 Host 口；
3. 在电脑端准备与板卡系统兼容的 `hdc`；
4. 从 GitHub Release 下载 `PqcMlkemDemo-rk3568-v1.0.2.hap`。

先确认设备在线：

```sh
hdc list targets
```

正常情况下会显示一行设备序列号。如果显示 `[Empty]`，先确认板卡已经启动并解锁，再重新插拔
OTG 数据线；仍无法识别时，可以给板卡完整断电十秒后重启。

## 安装并启动

```sh
hdc install -r PqcMlkemDemo-rk3568-v1.0.2.hap
hdc shell aa start -a EntryAbility -b com.example.pqcmlkem
```

如果已经克隆本仓库，也可以执行：

```sh
HDC_BIN=/path/to/hdc ./scripts/run_board_demo.sh PqcMlkemDemo-rk3568-v1.0.2.hap
```

安装命令出现 `install bundle successfully`，启动命令出现 `start ability successfully`，说明应用
已经正常部署。如果板卡停在锁屏界面，需要先上滑解锁。

## 演示步骤

1. 在“输入实验消息”区域输入任意中文或英文内容；
2. 点击“生成密钥并加密”；
3. 查看加密前明文、完整消息密文、ML-KEM 密文摘要、Nonce、认证标签和操作耗时；
4. 点击“解密并校验”；
5. 查看恢复明文、双方共享密钥是否一致，以及完整性认证是否通过；
6. 向下滚动到“方法效果对比”，查看正式实验数据；
7. 如需现场重新测量，点击“重新现场测试”。正式模式为 5 轮，每项每轮 10000 次。

## 预期结果

- 解密后的明文应与输入完全一致；
- 发送方和接收方的共享密钥指纹应一致；
- 未被修改的密文应通过完整性校验；
- AArch64/NEON 的三项平均耗时应低于可移植 C，但具体数值会受温度和系统负载影响。
