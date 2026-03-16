# Windows-Media-Player-9-For-Windows-7
Run Windows Media Player 9 on Windows 7(R) without setup_wm.exe support

# How to run
Run WMPConfigure.bat and follow steps it given. Do not modify any other files, especially for file names.

## 可能遇到的问题

### #1 这个脚本是用来做什么的？

    该脚本用于将 Windows 7 系统中的 Windows Media Player 12 强制降级为旧版本（ Windows Media Player 9），可以满足部分怀旧需求，搭配Windows XP Luna主题包效果应该更好，如果你有能力使用破解版主题相关dll文件的话。

### #2 为什么不直接替换原来的文件？

    如果这样做，Windows Media Player将报出DLL版本错误问题，虽然已经确定检查的是哪个DLL，但最终会报出内部应用程序错误。

### #3 能否保留WMC(Windows Media Center)？

    目前没有找到相关的方法，即使在dism命令中只指定了卸载WMP。

### #4 运行结束后，我是否还要保留该脚本？

    如果你考虑在多用户环境下保留脚本，那么应该保留脚本。但是，wmp和wmp9xp两个文件夹可以不保留。

### #5 运行这个脚本会有什么风险？

    除非运行还原点，否则你大概不能还原Windows Media Player。你当然还是可以使用“打开或关闭 Windows 功能”重新启用WMP和WMC，但这样做的结果是WMP直接报告安装不正确，WMC却能打开。

### #6 我能不能使用其他编解码器？

    可以，至少在我的电脑上如此。我是先安装K-Lite Codec Pack再替换WMP的，至少到现在为止，WMP和PotPlayer都能运行，而且解码flac等文件也没有遇到过问题。但安装过程混乱且单凭记忆给出结论，建议谨慎操作

### #7 脚本不能还原哪些旧版功能？

    **至少**包括：
    
    1) 最小模式不启动；
    2) 部分音频信息显示不正确（全乱码，测试文件为日文，结果只显示英文；文件资源管理器和WMP对音频图片解析不是一个模块完成的，前者显示，但后者不显示），这个问题在Windows XP中也存在；
    3) 并没有还原访问网页时的体验；
    4) “脱机工作”无法选中，更不能打开媒体指南；

### #8 该脚本支持的操作系统版本有哪些？

### #9 sfc /scannow命令还能不能正常使用？

### #10 要关闭杀毒软件吗？

    建议临时关闭并添加排除项，至少XP中安装360测试时连mplayer2.exe都报毒