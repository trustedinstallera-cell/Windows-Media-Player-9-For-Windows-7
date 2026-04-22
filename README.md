> [!WARNING]
>
> **Note for non-Chinese users:** The binary files in this project are Simplified Chinese targeted only and has only been fully tested on Windows 7 SP1 x64. To adapt it for your language, please refer to [LANGUAGE_REPLACEMENT.md](LANGUAGE_REPLACEMENT.md) for manual replacement (advanced users only).

> [!CAUTION]
> 杀毒软件可能会拦截系统文件操作，请提前添加信任。
>
> 该项目**完全以个人经验完成，请明确了解风险并谨慎使用该项目**

> [!CAUTION]
> **使用本工具需要一定的计算机基础**  
> 本工具涉及系统文件替换、注册表修改等操作，适用于熟悉以下概念的用户：
>
> - 管理员身份运行
> - 系统还原点
> - UAC（用户账户控制）
>
> 如果你不熟悉上述术语，建议请懂电脑的朋友协助操作。因操作不当导致的系统问题，作者不承担责任。

> [!WARNING]
> 本工具涉及系统文件替换，操作前请务必创建系统还原点。
>  ⚠️ **重要提醒**：操作前请备份数据。

> [!WARNING]
> Windows Media Center 的播放功能与Windows Media Player 不兼容。但其他大部分功能正常。

> [!TIP]
> 运行前建议先关闭所有正在运行的 Windows Media Player 与 Windows Media Center 进程。

## **项目简介**

本工具用于怀旧需要，快速将 Windows Media Player 12降级为Windows Media Player 9 Series。


## 💡 用法

1. 选择适用于您需要的版本 [Windows Media Player 9](https://github.com/trustedinstallera-cell/Windows-Media-Player-9-For-Windows-7/releases/download/v1.3/WMP9_Config.zip) 或 [Windows Media Player 10](https://github.com/trustedinstallera-cell/Windows-Media-Player-9-For-Windows-7/releases/download/v1.3/WMP10_Config.zip)并下载压缩包。**注意：程序不会判断使用的 WMP 版本并给出提示。**
2. 双击运行适用于您的系统架构的 WMP_Config.exe。如果不确定，可以随意选择一个版本，操作系统或程序将引导您选择正确的版本。
3. 按 1 键并回车开始配置，如果弹出 UAC 窗口，请点击“是”。再次按下 1 键并回车。程序将正式开始配置，看到重启提示后，请重新启动计算机。
4. 重启后再次运行程序，按 1 键完成部署。您还可以选择是否重新注册打开方式与还原部分快捷方式。
5. （可选）运行C:\Program Files\Windows Media Player\wmplayer.exe（x86系统）或 C:\Program Files (x86)\Windows Media Player\wmplayer.exe（x64版本）。

> [!Tip]
> 请注意事先保存您的工作，脚本要求计算机重新启动。

> [!Tip]
> 如果遇到任何不确定的选项，直接按Y即可

## **原理说明**

1、regsvr32可以用于注册具有DllRegisterServer接口的DLL类型文件；

2、DisablePCA的值为1时，可以禁用程序兼容性助手（也可以定位到gpedit.msc->本地计算机 策略\计算机配置\管理模板\Windows 组件\应用程序兼容性）；

3、对于本脚本涉及到的文件类型，均按照.*格式保存在HKCR中，其中(默认)值的数值数据存储了用于识别的类型，FriendlyTypeName负责配置文件类型描述，\DefaultIcon\(默认)负责配置文件图标

##  **注意事项**

> ⚠️ **重要提醒**
>
> - **仅提供了简体中文版本**：本工具默认文件为简体中文，其他语言系统无法直接使用（如需使用请参考 `LANGUAGE_REPLACEMENT.md`）。但是，无论在哪个语言的系统上，程序都应该可以运行（可能需要注意编码问题）。
> - **管理员权限**：所有操作必须以管理员身份运行，否则会失败。
> - **备份建议**：操作前建议创建系统还原点，以便恢复。
> - **杀毒软件**：部分杀软可能会拦截系统文件替换，请添加信任或暂时关闭。360 安全卫士将多次警告修改了系统设置与注册了dll文件，但 Microsoft Windows Defender 没有拦截程序的任何操作。
> - **兼容性问题**：该项目部分绕过了Windows兼容性选项，建议保留系统还原点并准备第三方播放器作为备用。
> - **使用许可问题**：滥用该项目可能违反用户许可协议。

## 系统要求
**最低要求：** Windows Vista

**推荐环境：** Windows 7 SP1 及以上的 x64 架构的操作系统

**稳定性说明：**

  - 64位系统（Windows 7 及以上）：最佳稳定性且Windows 7 SP1 x64已经经过长期使用
  - 32位系统（Windows 7 及以上）：稳定性良好
  - Windows Vista：基础功能可用，但稳定性未经充分测试且做法不符合常规流程并具有侵入性
**已经测试的系统：**
    Windows 7 SP1 x64、Windows Vista SP2 x64（不建议尝试）、Windows 8.1 x64
    Windows 10 version 1511 x86、Windows 10 version 1909 x64、Windows 11 25H2
    
**杂项：**
    不建议在 Windows XP Professional 中为运行该程序而安装 One-Core-API，虽然只有 dism 命令无法运行，但此举无任何正面效果，仅可作为测试用途，且作者不对该行为负责。
    Windows Media Player 10同样可用，且从 v1.3 开始同步二进制文件。当前发行版在 Windows 7 SP1 中正常运行。

> [!WARNING]
> 警告：程序**无法在 Windows Server 系列中正常完成工作**。若要强制运行，需要手动复制 wmp 文件夹中的文件，手动注册 dll 文件。而且，**系统还原点默认不可用**。**仅核心组件状态下缺少必要组件。**


## 对封面图片的限制

### 外挂图片

| 项目   | WMP 9情况                                                    |
| ------ | ------------------------------------------------------------ |
| 文件名 | 为folder.jpg，但不限制大小写； |
|  图片格式  | jpg格式；也可以是是png格式，但此时文件名仍然是folder.jpg  |
| 编码类型 | 仅支持基线 JPEG（Baseline）。渐进式 JPEG（Progressive）无法显示 |
| 色彩深度 | **尚未测试**，但 24 位 RGB 可行|
| 分辨率 | 无硬性限制，40\*40 到 11364\*4221 均可显示 |
| 文件大小 | 无明确限制，1501 字节到 10.3 MB 均可显示 |
| 存放位置 | 与音频文件同一目录|
| 优先级 | 高于内嵌图片 |

## **常见问题**

### 1、我能不能使用setup_wm.exe？

不能，除非关闭应用程序兼容性引擎并重新启动计算机，但是你只能得到[这样的](https://github.com/trustedinstallera-cell/Windows-Media-Player-9-For-Windows-7/blob/main/Screenshots/Incorrect-Result/Failed%20to%20install%20with%20capability%20options%20disabled.png)结果。高帧率录屏测试显示，程序只能运行到Skins或注册Windows Media Player组件即终止。可以部分显示初次运行wmplayer.exe的界面，但代价是无法配置任何应用程序的兼容性。

### 2、有没有多语言支持？

没有，这不是本脚本设置的初衷，而且只能选择一个语言版本进行配置。但是。如果有一定计算机基础，您也许可以参考[LANGUAGE_REPLACEMENT.md](LANGUAGE_REPLACEMENT.md) 。

### 3、我还能不能使用系统资源修复命令？
sfc /scannow 命令与 DISM.exe /Online /Cleanup-Image /ScanHealth 均未报告任何错误，重新启动计算机后运行 Windows Media Player 也没有遇到任何错误。

## 已知问题
1. 最小模式不启动；
2. “脱机工作”无法选中，媒体指南只能在断网状态下打开，否则会因为尝试访问 https://support.microsoft.com/zh-cn/topic 而连续报出多个脚本错误，单击“主页”也存在相同的问题。原因可能是 Windows Media Player 调用了 IE 内核。脚本错误弹窗问题在 Internet Explorer 8 版本中不存在，仅提示网页无法打开。*如果遇到脚本错误弹窗，按住Esc键的同时多次单击“正在播放”选项可以避开此错误产生的影响。* **该问题在冷启动 Windows Media Player 时总是复现**；
3. Aero 主题下的非默认样式可能有多出的窗口边框；
4. setup_wm.exe 无法启动；
5. 文件资源管理器预览功能不可用；
6. 只有在管理员权限下，媒体库功能才能正常工作。即使是管理员账户，也需要勾选“wmplayer.exe->属性->特权等级->以管理员身份运行此程序”才能生效，而且除非关闭 UAC，否则每次运行都会弹窗。普通账户则一定弹窗，除非放弃该功能。
7. 对某些USB设备的复制功能可能不生效。

## **免责声明**

替换系统文件存在风险，操作前请确保已备份。且该做法可能违反EULA。作者不对因使用该项目导致的一系列问题负责。
