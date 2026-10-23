import pqcMlkem from 'libpqc_mlkem_napi.so'

interface EncryptResult {
  success: boolean
  message: string
  ciphertextHex: string
  nonceHex: string
  authTagHex: string
  kemCiphertextPreview: string
  senderFingerprint: string
  messageBytes: number
  kemCiphertextBytes: number
  hybridCiphertextBytes: number
  keypairUs: number
  encapsUs: number
  dataEncryptUs: number
  totalEncryptUs: number
}

interface DecryptResult {
  success: boolean
  message: string
  plaintext: string
  authValid: boolean
  secretMatch: boolean
  receiverFingerprint: string
  decapsUs: number
  dataDecryptUs: number
  totalDecryptUs: number
}

interface CompareResult {
  success: boolean
  message: string
  iterations: number
  portableKeypairUs: number
  portableEncapsUs: number
  portableDecapsUs: number
  neonKeypairUs: number
  neonEncapsUs: number
  neonDecapsUs: number
  keypairImprovement: number
  encapsImprovement: number
  decapsImprovement: number
}

@Entry
@Component
struct Index {
  @State inputMessage: string = '你好，OpenHarmony 后量子密码！'
  @State encryptedInput: string = ''
  @State ciphertextHex: string = ''
  @State nonceHex: string = ''
  @State authTagHex: string = ''
  @State kemPreview: string = ''
  @State decryptedText: string = ''
  @State senderFingerprint: string = '----------------'
  @State receiverFingerprint: string = '----------------'
  @State encryptStatus: string = '等待输入消息'
  @State decryptStatus: string = '尚未解密'
  @State encrypting: boolean = false
  @State decrypting: boolean = false
  @State encrypted: boolean = false
  @State decrypted: boolean = false
  @State authValid: boolean = false
  @State secretMatch: boolean = false
  @State messageBytes: number = 0
  @State hybridBytes: number = 0
  @State keypairTime: string = '--'
  @State encapsTime: string = '--'
  @State messageEncryptTime: string = '--'
  @State decapsTime: string = '--'
  @State messageDecryptTime: string = '--'

  @State benchmarking: boolean = false
  @State benchmarkStatus: string = '正式测试：5 轮 × 每轮 10000 次'
  @State benchmarkIterations: number = 10000
  @State portableKeypair: string = '79.003'
  @State portableEncaps: string = '91.054'
  @State portableDecaps: string = '86.547'
  @State neonKeypair: string = '58.313'
  @State neonEncaps: string = '65.254'
  @State neonDecaps: string = '56.544'
  @State keypairImprovement: string = '26.19%'
  @State encapsImprovement: string = '28.34%'
  @State decapsImprovement: string = '34.67%'
  @State keypairBarWidth: string = '73.8%'
  @State encapsBarWidth: string = '71.7%'
  @State decapsBarWidth: string = '65.3%'

  private formatHex(value: string): string {
    let output: string = ''
    for (let index: number = 0; index < value.length; index += 32) {
      if (output.length > 0) {
        output += ' '
      }
      output += value.substring(index, Math.min(index + 32, value.length))
    }
    return output
  }

  private barWidth(portableValue: number, neonValue: number): string {
    if (portableValue <= 0) {
      return '100%'
    }
    return Math.min(100, neonValue * 100 / portableValue).toFixed(1) + '%'
  }

  private encryptMessage() {
    if (this.encrypting || this.inputMessage.length === 0) {
      this.encryptStatus = this.inputMessage.length === 0 ? '请先输入消息' : this.encryptStatus
      return
    }
    this.encrypting = true
    this.encrypted = false
    this.decrypted = false
    this.decryptStatus = '尚未解密'
    this.encryptStatus = '正在生成密钥并加密消息…'

    setTimeout(() => {
      try {
        let result = pqcMlkem.encryptMessage(this.inputMessage) as EncryptResult
        this.encryptStatus = result.message
        if (result.success) {
          this.encryptedInput = this.inputMessage
          this.ciphertextHex = this.formatHex(result.ciphertextHex)
          this.nonceHex = result.nonceHex
          this.authTagHex = this.formatHex(result.authTagHex)
          this.kemPreview = this.formatHex(result.kemCiphertextPreview)
          this.senderFingerprint = result.senderFingerprint
          this.messageBytes = result.messageBytes
          this.hybridBytes = result.hybridCiphertextBytes
          this.keypairTime = result.keypairUs.toFixed(3)
          this.encapsTime = result.encapsUs.toFixed(3)
          this.messageEncryptTime = result.dataEncryptUs.toFixed(3)
          this.encrypted = true
        }
      } catch (error) {
        this.encryptStatus = '原生加密模块调用失败：' + String(error)
      }
      this.encrypting = false
    }, 60)
  }

  private decryptMessage() {
    if (!this.encrypted || this.decrypting) {
      return
    }
    this.decrypting = true
    this.decrypted = false
    this.decryptStatus = '正在解封装共享密钥并恢复明文…'

    setTimeout(() => {
      try {
        let result = pqcMlkem.decryptMessage() as DecryptResult
        this.decryptStatus = result.message
        this.decryptedText = result.plaintext
        this.authValid = result.authValid
        this.secretMatch = result.secretMatch
        this.receiverFingerprint = result.receiverFingerprint
        this.decapsTime = result.decapsUs.toFixed(3)
        this.messageDecryptTime = result.dataDecryptUs.toFixed(3)
        this.decrypted = result.success
      } catch (error) {
        this.decryptStatus = '原生解密模块调用失败：' + String(error)
      }
      this.decrypting = false
    }, 60)
  }

  private compareMethods() {
    if (this.benchmarking) {
      return
    }
    this.benchmarking = true
    this.benchmarkStatus = '正在依次测试可移植 C 与 NEON…'

    setTimeout(() => {
      try {
        let result = pqcMlkem.compareMethods(500) as CompareResult
        this.benchmarkStatus = result.message
        if (result.success) {
          this.benchmarkIterations = result.iterations
          this.portableKeypair = result.portableKeypairUs.toFixed(3)
          this.portableEncaps = result.portableEncapsUs.toFixed(3)
          this.portableDecaps = result.portableDecapsUs.toFixed(3)
          this.neonKeypair = result.neonKeypairUs.toFixed(3)
          this.neonEncaps = result.neonEncapsUs.toFixed(3)
          this.neonDecaps = result.neonDecapsUs.toFixed(3)
          this.keypairImprovement = result.keypairImprovement.toFixed(2) + '%'
          this.encapsImprovement = result.encapsImprovement.toFixed(2) + '%'
          this.decapsImprovement = result.decapsImprovement.toFixed(2) + '%'
          this.keypairBarWidth = this.barWidth(result.portableKeypairUs, result.neonKeypairUs)
          this.encapsBarWidth = this.barWidth(result.portableEncapsUs, result.neonEncapsUs)
          this.decapsBarWidth = this.barWidth(result.portableDecapsUs, result.neonDecapsUs)
        }
      } catch (error) {
        this.benchmarkStatus = '性能测试调用失败：' + String(error)
      }
      this.benchmarking = false
    }, 60)
  }

  @Builder
  Tag(text: string, color: string) {
    Text(text)
      .fontSize(13)
      .fontColor(color)
      .fontWeight(FontWeight.Medium)
      .padding({ left: 11, right: 11, top: 5, bottom: 5 })
      .backgroundColor('#13243D')
      .borderRadius(15)
  }

  @Builder
  DataBox(title: string, content: string, color: string) {
    Column({ space: 7 }) {
      Text(title)
        .width('100%')
        .fontSize(13)
        .fontColor('#8FA4C2')
      Text(content)
        .width('100%')
        .fontSize(15)
        .fontColor(color)
    }
    .width('100%')
    .alignItems(HorizontalAlign.Start)
    .padding(14)
    .backgroundColor('#091525')
    .borderRadius(12)
    .border({ width: 1, color: '#1C324E' })
  }

  @Builder
  TimingBox(title: string, value: string) {
    Column({ space: 4 }) {
      Text(title).fontSize(12).fontColor('#8FA4C2')
      Text(value + ' μs')
        .fontSize(18)
        .fontWeight(FontWeight.Bold)
        .fontColor('#55E6C1')
    }
    .layoutWeight(1)
    .padding(12)
    .backgroundColor('#091525')
    .borderRadius(11)
  }

  @Builder
  PerformanceCard(title: string, baseline: string, neon: string,
    improvement: string, neonWidth: string) {
    Column({ space: 10 }) {
      Row() {
        Text(title).fontSize(17).fontWeight(FontWeight.Bold).fontColor('#F3F7FF')
        Blank()
        Text('耗时降低 ' + improvement).fontSize(13).fontColor('#55E6C1')
      }
      .width('100%')

      Row({ space: 9 }) {
        Text('可移植 C').width(70).fontSize(12).fontColor('#8FA4C2')
        Row().layoutWeight(1).height(10).backgroundColor('#1C3556').borderRadius(5)
        Text(baseline + ' μs').width(90).fontSize(12).fontColor('#B7C6DB')
          .textAlign(TextAlign.End)
      }
      .width('100%')

      Row({ space: 9 }) {
        Text('NEON').width(70).fontSize(12).fontColor('#55E6C1')
        Stack({ alignContent: Alignment.Start }) {
          Row().width(neonWidth).height(10).backgroundColor('#19C6A3').borderRadius(5)
        }
        .layoutWeight(1)
        Text(neon + ' μs').width(90).fontSize(12).fontColor('#55E6C1')
          .textAlign(TextAlign.End)
      }
      .width('100%')
    }
    .width('100%')
    .padding(15)
    .backgroundColor('#0F1D31')
    .borderRadius(15)
    .border({ width: 1, color: '#203653' })
  }

  build() {
    Scroll() {
      Column({ space: 18 }) {
        Row() {
          Column({ space: 6 }) {
            Text('OPENHARMONY · PQC LAB')
              .fontSize(13).fontColor('#55E6C1').fontWeight(FontWeight.Medium)
            Text('ML-KEM 消息加解密演示')
              .fontSize(27).fontWeight(FontWeight.Bold).fontColor('#F3F7FF')
            Text('DAYU200 · RK3568 · 真实原生算法调用')
              .fontSize(14).fontColor('#8FA4C2')
          }
          Blank()
          Text('PQC')
            .width(62).height(62).textAlign(TextAlign.Center)
            .fontSize(21).fontWeight(FontWeight.Bold).fontColor('#07111F')
            .backgroundColor('#55E6C1').borderRadius(18)
        }
        .width('100%')

        Row({ space: 9 }) {
          this.Tag('ML-KEM-512', '#55E6C1')
          this.Tag('KEM + DEM', '#7FB0FF')
          this.Tag('SHAKE256', '#7FB0FF')
          this.Tag('NEON', '#FFBE70')
        }
        .width('100%')

        Column({ space: 13 }) {
          Row() {
            Text('① 输入实验消息')
              .fontSize(20).fontWeight(FontWeight.Bold).fontColor('#F3F7FF')
            Blank()
            Text('最多 160 字符').fontSize(12).fontColor('#6F86A6')
          }
          .width('100%')
          TextArea({ placeholder: '请在这里输入要加密的明文', text: this.inputMessage })
            .width('100%').height(104).fontSize(17).fontColor('#F3F7FF')
            .placeholderColor('#607895').backgroundColor('#091525')
            .borderRadius(13).padding(13).maxLength(160)
            .onChange((value: string) => {
              this.inputMessage = value
            })
          Text('ML-KEM 协商 32 B 共享密钥，SHAKE256 生成消息密钥流，SHA3-256 校验完整性。')
            .width('100%').fontSize(12).fontColor('#6F86A6')
          Row({ space: 12 }) {
            Button(this.encrypting ? '正在加密…' : '生成密钥并加密')
              .layoutWeight(1).height(52).fontSize(17).fontWeight(FontWeight.Bold)
              .fontColor('#07111F').backgroundColor(this.encrypting ? '#6E8198' : '#55E6C1')
              .borderRadius(14).enabled(!this.encrypting)
              .onClick(() => this.encryptMessage())
            Button(this.decrypting ? '正在解密…' : '解密并校验')
              .layoutWeight(1).height(52).fontSize(17).fontWeight(FontWeight.Bold)
              .fontColor(this.encrypted ? '#F3F7FF' : '#73859D')
              .backgroundColor(this.encrypted ? '#1A4E7A' : '#172438')
              .borderRadius(14).enabled(this.encrypted && !this.decrypting)
              .onClick(() => this.decryptMessage())
          }
          .width('100%')
          Text(this.encryptStatus)
            .width('100%').fontSize(13)
            .fontColor(this.encrypted ? '#55E6C1' : '#FFBE70')
        }
        .width('100%').padding(19).backgroundColor('#0B1728').borderRadius(19)

        if (this.encrypted) {
          Column({ space: 12 }) {
            Row() {
              Text('② 加密效果').fontSize(20).fontWeight(FontWeight.Bold).fontColor('#F3F7FF')
              Blank()
              Text(this.messageBytes + ' B 明文 → ' + this.hybridBytes + ' B 完整载荷')
                .fontSize(12).fontColor('#55E6C1')
            }
            .width('100%')
            this.DataBox('加密前的明文', this.encryptedInput, '#F3F7FF')
            this.DataBox('消息密文（完整十六进制）', this.ciphertextHex, '#FFBE70')
            this.DataBox('ML-KEM 密文前 32 B（完整 KEM 密文为 768 B）', this.kemPreview + ' …', '#7FB0FF')
            this.DataBox('随机数 Nonce', this.nonceHex, '#B6A5FF')
            this.DataBox('SHA3-256 认证标签', this.authTagHex, '#B6A5FF')
            Row({ space: 10 }) {
              this.TimingBox('密钥生成', this.keypairTime)
              this.TimingBox('密钥封装', this.encapsTime)
              this.TimingBox('消息加密', this.messageEncryptTime)
            }
            .width('100%')
            Text('发送方共享密钥指纹：' + this.senderFingerprint)
              .width('100%').fontSize(13).fontColor('#8FA4C2')
          }
          .width('100%').padding(19).backgroundColor('#0B1728').borderRadius(19)
        }

        if (this.decrypted) {
          Column({ space: 12 }) {
            Row() {
              Text('③ 解密结果').fontSize(20).fontWeight(FontWeight.Bold).fontColor('#F3F7FF')
              Blank()
              Text(this.decryptedText === this.encryptedInput ? '✓ 明文完全一致' : '✕ 明文不一致')
                .fontSize(14).fontWeight(FontWeight.Bold)
                .fontColor(this.decryptedText === this.encryptedInput ? '#55E6C1' : '#FF6978')
            }
            .width('100%')
            this.DataBox('解密后恢复的明文', this.decryptedText, '#55E6C1')
            Row({ space: 12 }) {
              Column({ space: 5 }) {
                Text('共享密钥').fontSize(12).fontColor('#8FA4C2')
                Text(this.secretMatch ? '✓ 双方一致' : '✕ 不一致')
                  .fontSize(16).fontWeight(FontWeight.Bold)
                  .fontColor(this.secretMatch ? '#55E6C1' : '#FF6978')
              }.layoutWeight(1)
              Column({ space: 5 }) {
                Text('完整性认证').fontSize(12).fontColor('#8FA4C2')
                Text(this.authValid ? '✓ 校验通过' : '✕ 校验失败')
                  .fontSize(16).fontWeight(FontWeight.Bold)
                  .fontColor(this.authValid ? '#55E6C1' : '#FF6978')
              }.layoutWeight(1)
              Column({ space: 5 }) {
                Text('接收方指纹').fontSize(12).fontColor('#8FA4C2')
                Text(this.receiverFingerprint).fontSize(14).fontColor('#F3F7FF')
              }.layoutWeight(1)
            }
            .width('100%')
            Row({ space: 10 }) {
              this.TimingBox('密钥解封装', this.decapsTime)
              this.TimingBox('消息解密与认证', this.messageDecryptTime)
            }
            .width('100%')
            Text(this.decryptStatus).width('100%').fontSize(13).fontColor('#55E6C1')
          }
          .width('100%').padding(19).backgroundColor('#0B1728').borderRadius(19)
        }

        Column({ space: 13 }) {
          Row() {
            Column({ space: 4 }) {
              Text('④ 方法效果对比')
                .fontSize(20).fontWeight(FontWeight.Bold).fontColor('#F3F7FF')
              Text(this.benchmarkStatus).fontSize(12).fontColor('#8FA4C2')
            }
            Blank()
            Button(this.benchmarking ? '测试中…' : '重新现场测试')
              .height(42).fontSize(14).fontColor('#07111F')
              .backgroundColor(this.benchmarking ? '#6E8198' : '#55E6C1')
              .borderRadius(12).enabled(!this.benchmarking)
              .onClick(() => this.compareMethods())
          }
          .width('100%')
          Text('相同 ML-KEM-512 参数与测试流程；每项 ' + this.benchmarkIterations + ' 次，耗时越低越好。')
            .width('100%').fontSize(12).fontColor('#6F86A6')
          this.PerformanceCard('密钥生成', this.portableKeypair, this.neonKeypair,
            this.keypairImprovement, this.keypairBarWidth)
          this.PerformanceCard('密钥封装', this.portableEncaps, this.neonEncaps,
            this.encapsImprovement, this.encapsBarWidth)
          this.PerformanceCard('密钥解封装', this.portableDecaps, this.neonDecaps,
            this.decapsImprovement, this.decapsBarWidth)
        }
        .width('100%').padding(19).backgroundColor('#0B1728').borderRadius(19)

        Text('教学实验用 KEM-DEM 演示 · 未声明为经过安全认证的商用密码模块')
          .width('100%').textAlign(TextAlign.Center).fontSize(12).fontColor('#536B89')
      }
      .width('100%').padding({ left: 20, right: 20, top: 24, bottom: 34 })
    }
    .width('100%').height('100%').backgroundColor('#07111F')
  }
}
