> [!WARNING]
>
> **Note for non-Chinese users:** The binary files in this project are Simplified Chinese targeted only and has only been fully tested on Windows 7 SP1 x64. To adapt it for your language, please refer to [LANGUAGE_REPLACEMENT.md](LANGUAGE_REPLACEMENT.md) for manual replacement (advanced users only).

> [!CAUTION]
> 杀毒软件可能会拦截系统文件操作，请提前添加信任。
>
> 该项目**仅在Windows 7 SP1 x64 Ultimate中测试通过，请谨慎使用该项目**

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
> 本工具涉及系统文件替换，操作前请务必创建系统还原点。 ⚠️ **重要提醒**：操作前请备份数据。

> [!TIP]
> 运行前建议先关闭所有正在运行的 Windows Media Player 进程。

## **项目简介**

本工具用于怀旧需要，快速将 Windows Media Player 12降级为Windows Media Player 9 Series。

## 💡 用法

1、以管理员身份运行该脚本，按1键以开始配置直到脚本提示重新启动计算机。
2、再次打开脚本，按1键，脚本将开始部署 WMP 直到预设的选项所在位置。

> [!Tip]
> 请注意事先保存您的工作，脚本要求计算机重新启动。

> [!TIP]
> 如果遇到任何不确定的选项，直接按Y即可

## **原理说明**

1、regsvr32可以用于注册具有DllRegisterServer接口的DLL类型文件；
2、DisablePCA的值为1时，可以禁用程序兼容性助手（也可以定位到gpedit.msc->本地计算机 策略\计算机配置\管理模板\Windows 组件\应用程序兼容性；
3、对于本脚本涉及到的文件类型，均按照.*格式保存在HKCR中，其中(默认)值的数值数据存储了用于识别的类型，FriendlyTypeName负责配置文件类型描述，\DefaultIcon\(默认)负责配置文件图标

##  **注意事项**

> ⚠️ **重要提醒**
>
> - **仅适用于简体中文版 Windows**：本工具默认文件为简体中文，其他语言系统无法直接使用（如需使用请参考 `LANGUAGE_REPLACEMENT.md`）。
> - **管理员权限**：所有操作必须以管理员身份运行，否则会失败。
> - **备份建议**：操作前建议创建系统还原点，以便恢复。
> - **杀毒软件**：部分杀软可能会拦截系统文件替换，请添加信任或暂时关闭。

## 系统要求
目前仅测试过 Windows 7 SP1 x64，也是为该系统设计的。

## **常见问题**

### 1、我能不能使用setup_wm.exe？

不能，除非关闭应用程序兼容性引擎并重新启动计算机，但是你只能得到这样的图片所示的结果。高帧率录屏测试显示，程序只能运行到Skins或注册Windows Media Player组件即终止。可以部分显示初次运行wmplayer.exe的界面，但代价是无法配置任何应用程序的兼容性。

### 2、有没有多语言支持？

没有，这不是本脚本设置的初衷，而且只能选择一个语言版本进行配置。但是。如果有一定计算机基础，您也许可以参考[LANGUAGE_REPLACEMENT.md](LANGUAGE_REPLACEMENT.md) 。

## 已知问题
1. 最小模式不启动；
2. “脱机工作”无法选中，更不能打开媒体指南；
3. Aero 主题下的非默认样式可能有多出的窗口边框；
4. setup_wm.exe 无法启动
5. 文件资源管理器预览功能不可用

## **免责声明**

替换系统文件存在风险，操作前请确保已备份。且该做法可能违反EULA。作者不对因使用该项目导致的一系列问题负责。
