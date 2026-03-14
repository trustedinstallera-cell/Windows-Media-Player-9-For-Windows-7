@echo off
cd /d "%~dp0"
setlocal enabledelayedexpansion
rem 进行调优工作，还是进行覆写安装？
ver | find "6.1" > nul
if errorlevel 1 (
    echo Warning: 不是被测试通过的版本
    echo Info: 若选择继续，将导致无法预料的情况
)
set /a Exception=0
if not exist "wmp9xp" (
	echo Error:找不到名为 wmp9xp 的文件夹，请重新下载程序
	set /a Exception=1
)
if not exist "wmp" (
	echo Error:找不到名为 wmp 的文件夹，请重新下载程序
	set /a Exception=1
)
cd /d "%~dp0"
openfiles >nul 2>&1
if '%errorlevel%' == '0' goto run

:: 生成一个随机的临时文件名
:generate
set "vbs=%temp%\~tmpadmin%RANDOM%.vbs"
if exist "%vbs%" goto generate

:: 写入VBS并执行
echo Set UAC = CreateObject^("Shell.Application"^) > "%vbs%"
echo UAC.ShellExecute "%~s0", "", "", "runas", 1 >> "%vbs%"
"%vbs%"

:: 等待一小段时间再删除文件（防止文件被占用）
ping -n 2 127.0.0.1 >nul
del /f /q "%vbs%" >nul 2>&1
exit /b

:run

if "%Exception%" == "1" (
    echo Info: 程序将会退出
    pause
    exit /b
)

pause
if not exist "1" (
	tasklist | find /i "wmplayer.exe" >nul
	if %errorlevel% equ 0 (
		echo 需要关闭 Windows Media Player ，正在关闭...
		taskkill /f /im wmplayer.exe
		if %errorlevel% equ 1 (
			echo 错误：找不到名为 wmplayer.exe 的进程或权限不足。
			echo 请确认程序没有运行后按任意键。
			pause
		)
	)
	tasklist | find /i "ehshell.exe" >nul
	if %errorlevel% equ 0 (
		echo 需要关闭 Windows Media Center ，正在关闭...
		taskkill /f /im ehshell.exe
		if %errorlevel% equ 1 (
			echo 错误：找不到名为 ehshell.exe 的进程或权限不足。
			echo 请确认程序没有运行后按任意键。
			pause
		)
	)
	rem 创建系统还原点...
	echo 强烈建议您在运行前创建系统还原点或备份文件，因为该脚本会直接修改系统配置，可能影响系统稳定性
	choice /c yn /m "创建系统还原点？"
    if errorlevel 2 (
        echo 跳过还原点配置
    ) else (
	:BeginSystemRestore
        echo 正在启用系统还原...
		wmic /namespace:\\root\default path SystemRestore call Enable "C:"
		if %errorlevel% equ 0 (
			echo 成功启用系统还原。
			wmic /namespace:\\root\default path SystemRestore call CreateRestorePoint "降级至 Windows Media Player 9", 0, 12
			::0：BEGIN_SYSTEM_CHANGE（开始系统更改）
			::12：MODIFY_SETTINGS（应用程序安装/设置更改）
			if %errorlevel% neq 0 (
				echo Error: 发生意外错误，错误代码：%errorlevel%
				choice /c yn /m "跳过还原点配置？"
				if errorlevel 2 (
					echo 跳过还原点配置
					goto EndSystemRestore
				) else (
					goto BeginSystemRestore
				)
				pause
			)
			if %errorlevel% equ 102 (
				echo Warning: 磁盘空间可能不足。
				echo Info: 本程序除了还原点空间外，还需要额外 50MB 大小的空间以容纳复制的组件，可能需要清理空间。
				pause
			)
		) else (
			echo 操作失败，错误代码：%errorlevel%
		)
    )
	:EndSystemRestore
    echo 卸载 Windows Media Center...
    start /w pkgmgr /uu:MediaCenter /quiet /norestart
    echo 卸载 Windows Media Player...
    start /w pkgmgr /uu:WindowsMediaPlayer /quiet /norestart
    echo > 1
    echo 第一阶段已完成。请尽快保存手头的工作，重新启动计算机，然后再次运行该程序。
	rem 现在就重新启动计算机吗？
	pause
	exit
) else (
    if exist "C:\Program Files (x86)\Windows Media Player" (
        echo 正在删除残留的文件...
        rmdir /S /Q "C:\Program Files (x86)\Windows Media Player"
    )
	echo 准备复制新文件，建议在 C 盘预留 50~100MB 空间。
	for /f "tokens=2 delims==" %%a in ('wmic logicaldisk where DeviceID^="C:" get FreeSpace /value ^| find "="') do set freebytes=%%a > nul
	echo 当前剩余空间为 %freebytes% 字节。
	pause
    echo 复制新文件...
    xcopy ".\wmp9xp\*.*" "C:\Program Files (x86)\Windows Media Player\" /E /I /Y
    echo 禁用程序兼容性助手...
    reg add "HKLM\SOFTWARE\Policies\Microsoft\Windows\AppCompat" /v DisablePCA /t REG_DWORD /d 1 /f
    reg add "HKLM\SOFTWARE\Wow6432Node\Policies\Microsoft\Windows\AppCompat" /v DisablePCA /t REG_DWORD /d 1 /f
    echo 复制新文件...
    xcopy ".\wmp\*.*" "C:\Program Files (x86)\Windows Media Player\" /E /I /Y
    echo 注册文件...
    for %%i in ("C:\Program Files (x86)\Windows Media Player\*.dll") do (
        echo 正在注册 %%i...
        regsvr32 /s "%%i"
    )
	for %%i in ("C:\Program Files (x86)\Windows Media Player\*.ax") do (
        echo 正在注册 %%i...
        regsvr32 /s "%%i"
    )
	echo 跳过安装向导，setup_wm.exe 不可用...
	reg add "HKCU\Software\Microsoft\MediaPlayer\Setup" /v "SetupCompleted" /t REG_DWORD /d 1 /f
	reg add "HKCU\Software\Microsoft\MediaPlayer\WOW6432Node\Setup" /v "SetupCompleted" /t REG_DWORD /d 1 /f 2>nul
    ::For mplayer2.exe
    echo 正在注册 msdxm.ocx...
    regsvr32 /s "C:\Program Files (x86)\Windows Media Player\msdxm.ocx"
    
	::Bypass setup_wm.exe using reg.exe
	echo 正在跳过 setup_wm.exe 部署过程，因为它被程序兼容性助手所阻止...
	
		
	if exist GetCurrentSID.exe (
		:: 获取当前用户 SID
		"GetCurrentSID.exe" > nul
		set /p SID=<wmp_sid.txt
		echo 当前用户 SID: !SID!
	) else (
		echo 获取 SID 失败。
		echo 如果您之前已运行过 Windows Media Player，可忽略此错误。
		echo 否则，请先获取当前系统的 SID，然后执行以下操作之一：
		echo - 编辑附带的 SkippingSetup_wm.reg 文件，将其中的示例 SID（S-1-5-21-1061874622-3929897800-1008325475-1000）替换为您的实际 SID，保存并导入注册表。
		echo - 或者，使用系统还原点将计算机还原到之前状态，然后运行 "C:\Program Files (x86)\Windows Media Player\setup_wm.exe"，并完全重新开始此安装程序。
		set /p SID="请通过你可以使用的方法获取 SID 并输入值，并使用附带的SkippingSetup_wm.reg，将 S-1-5-21-1061874622-3929897800-1008325475-1000 替换为您的 SID，保存并导入该注册表项；或使用系统还原点还原计算机，运行C:\Program Files (x86)\Windows Media Player\setum_wm.exe，然后从头运行该程序；如果您之前运行过 Windows Media Player，那么可以忽略此错误。"
		pause
	)
		
	:: 目标注册表路径
	set "KEY=HKU\!SID!\Software\Microsoft\MediaPlayer\Preferences"
	
	
	:: 写入所有必需的注册表键值（强制覆盖）
	reg add "!KEY!" /v "AcceptedPrivacyStatement"          /t REG_DWORD /d 1 /f
	reg add "!KEY!" /v "AppColorLimited"                   /t REG_DWORD /d 0 /f
	reg add "!KEY!" /v "DefaultSubscriptionService"        /t REG_SZ    /d "Bing" /f
	reg add "!KEY!" /v "DisableMRUMusic"                   /t REG_DWORD /d 0 /f
	reg add "!KEY!" /v "DisableMRUPictures"                /t REG_DWORD /d 0 /f
	reg add "!KEY!" /v "DisableMRUPlaylists"               /t REG_DWORD /d 0 /f
	reg add "!KEY!" /v "DisableMRUVideo"                    /t REG_DWORD /d 0 /f
	reg add "!KEY!" /v "EverLoadedServices"                /t REG_DWORD /d 1 /f
	reg add "!KEY!" /v "LastContainerMode"                 /t REG_DWORD /d 0 /f
	reg add "!KEY!" /v "LastContainerV12"                  /t REG_SZ    /d "{70C02500-7C6F-11D3-9FB6-00105AA620BB}" /f
	reg add "!KEY!" /v "LaunchIndex"                        /t REG_DWORD /d 1 /f
	reg add "!KEY!" /v "LibraryBackgroundImage"             /t REG_DWORD /d 6 /f
	reg add "!KEY!" /v "LibraryForceShowColumns"           /t REG_DWORD /d 0 /f
	reg add "!KEY!" /v "LibraryHasBeenPopulated"           /t REG_DWORD /d 1 /f
	reg add "!KEY!" /v "LibraryHMENodesVisible"            /t REG_DWORD /d 1 /f
	reg add "!KEY!" /v "MetadataRetrieval"                 /t REG_DWORD /d 3 /f
	reg add "!KEY!" /v "MigratedXML"                        /t REG_DWORD /d 1 /f
	reg add "!KEY!" /v "Migrating"                          /t REG_DWORD /d 0 /f
	rem 0x54 = 84
	reg add "!KEY!" /v "MLSChangeIndexList"                /t REG_DWORD /d 84 /f
	reg add "!KEY!" /v "MLSChangeIndexMusic"               /t REG_DWORD /d 3 /f
	reg add "!KEY!" /v "MLSChangeIndexPhoto"               /t REG_DWORD /d 8 /f
	reg add "!KEY!" /v "MLSChangeIndexVideo"               /t REG_DWORD /d 2 /f
	reg add "!KEY!" /v "MostRecentFileAddOrRemove"         /t REG_BINARY /d 10cb19e521b2dc01 /f
	reg add "!KEY!" /v "SendUserGUID"                       /t REG_BINARY /d 00 /f
	reg add "!KEY!" /v "SetHMEPermissionsOnDBDone"         /t REG_DWORD /d 1 /f
	reg add "!KEY!" /v "SilentAcquisition"                 /t REG_DWORD /d 1 /f
	reg add "!KEY!" /v "SQMLaunchIndex"                     /t REG_DWORD /d 1 /f
	reg add "!KEY!" /v "TranscodedFilesCacheDefaultSizeSet" /t REG_DWORD /d 1 /f
	rem 0x17ff = 6143
	reg add "!KEY!" /v "TranscodedFilesCacheSize"           /t REG_DWORD /d 6143 /f
	reg add "!KEY!" /v "TreeQueryWatcher"                   /t REG_DWORD /d 2 /f
	reg add "!KEY!" /v "UsageLoggerRanOnce"                 /t REG_DWORD /d 1 /f
	reg add "!KEY!" /v "UsageTracking"                      /t REG_DWORD /d 1 /f
	
	echo 注册表项已添加完成，Windows Media Player 将跳过首次运行配置。
	
    choice /c yn /m "重新注册快捷方式？"
    if errorlevel 2 (
        echo 跳过快捷方式注册
    ) else (
        echo 正在注册快捷方式...
		xcopy ".\Windows Media Player.lnk" "%ProgramData%\Microsoft\Windows\Start Menu\Programs" /E /Y
		
		::注意：Windows XP可以运行程序，但什么也不会发生
		PinToTaskbar.exe > nul
		if %errorlevel% equ 0 (
			echo 错误：找不到 PinToTaskbar.exe 或 Windows Media Player.lnk。若要固定到任务栏，请尝试手动执行。
		)
		pause
    )
    
    choice /c yn /m "重新注册打开方式？"
    if errorlevel 2 (
        echo 跳过打开方式注册
    ) else (
        echo 正在注册打开方式...
        rem 在这里添加打开方式注册代码
		rem 分开音视频逻辑
		:: 定义基础路径
		set "BASE=HKU\!SID!\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts"

		echo 正在配置音频文件关联...
		call :SetAssociation .aac   WMP11.AssocFile.ADTS !BASE!
		call :SetAssociation .adt   WMP11.AssocFile.ADTS !BASE!
		call :SetAssociation .adts  WMP11.AssocFile.ADTS !BASE!
		call :SetAssociation .aif   WMP11.AssocFile.AIFF !BASE!
		call :SetAssociation .aifc  WMP11.AssocFile.AIFF !BASE!
		call :SetAssociation .aiff  WMP11.AssocFile.AIFF !BASE!
		call :SetAssociation .au    WMP11.AssocFile.AU !BASE!
		call :SetAssociation .cda   WMP11.AssocFile.CDA !BASE!
		call :SetAssociation .m3u   WMP11.AssocFile.m3u !BASE!
		call :SetAssociation .m4a   WMP11.AssocFile.M4A !BASE!
		call :SetAssociation .mid   WMP11.AssocFile.MIDI !BASE!
		call :SetAssociation .midi  WMP11.AssocFile.MIDI !BASE!
		call :SetAssociation .mp2   WMP11.AssocFile.MP3 !BASE!
		call :SetAssociation .mp3   WMP11.AssocFile.MP3 !BASE!
		call :SetAssociation .rmi   WMP11.AssocFile.MIDI !BASE!
		call :SetAssociation .snd   WMP11.AssocFile.AU !BASE!
		call :SetAssociation .wav   WMP11.AssocFile.WAV !BASE!
		call :SetAssociation .wax   WMP11.AssocFile.WAX !BASE!
		call :SetAssociation .wma   WMP11.AssocFile.WMA !BASE!
		call :SetAssociation .wpl   WMP11.AssocFile.WPL !BASE!
		call :SetAssociation .wvx   WMP11.AssocFile.WVX !BASE!
		
		:: ==================== 视频文件关联 ====================
		echo 正在配置视频文件关联...
		call :SetAssociation .3g2   WMP11.AssocFile.3G2 !BASE!
		call :SetAssociation .3gp   WMP11.AssocFile.3GP !BASE!
		call :SetAssociation .asf   WMP11.AssocFile.ASF !BASE!
		call :SetAssociation .asx   WMP11.AssocFile.ASX !BASE!
		call :SetAssociation .avi   WMP11.AssocFile.AVI !BASE!
		call :SetAssociation .m1v   WMP11.AssocFile.MPEG !BASE!
		call :SetAssociation .m2t   WMP11.AssocFile.M2TS !BASE!
		call :SetAssociation .m2ts  WMP11.AssocFile.M2TS !BASE!
		call :SetAssociation .m2v   WMP11.AssocFile.MPEG !BASE!
		call :SetAssociation .m4v   WMP11.AssocFile.MP4 !BASE!
		call :SetAssociation .mod   WMP11.AssocFile.MPEG !BASE!
		call :SetAssociation .mov   WMP11.AssocFile.MOV !BASE!
		call :SetAssociation .mp2v  WMP11.AssocFile.MPEG !BASE!
		call :SetAssociation .mp4   WMP11.AssocFile.MP4 !BASE!
		call :SetAssociation .mp4v  WMP11.AssocFile.MP4 !BASE!
		call :SetAssociation .mpa   WMP11.AssocFile.MPEG !BASE!
		call :SetAssociation .mpe   WMP11.AssocFile.MPEG !BASE!
		call :SetAssociation .mpeg  WMP11.AssocFile.MPEG !BASE!
		call :SetAssociation .mpg   WMP11.AssocFile.MPEG !BASE!
		call :SetAssociation .mpv2  WMP11.AssocFile.MPEG !BASE!
		call :SetAssociation .mts   WMP11.AssocFile.M2TS !BASE!
		call :SetAssociation .ts    WMP11.AssocFile.TTS !BASE!
		call :SetAssociation .tts   WMP11.AssocFile.TTS !BASE!
		call :SetAssociation .wm    WMP11.AssocFile.ASF !BASE!
		call :SetAssociation .wmd   WMP11.AssocFile.WMD !BASE!
		call :SetAssociation .wms   WMP11.AssocFile.WMS !BASE!
		call :SetAssociation .wmv   WMP11.AssocFile.WMV !BASE!
		call :SetAssociation .wmx   WMP11.AssocFile.ASX !BASE!
		call :SetAssociation .wmz   WMP11.AssocFile.WMZ !BASE!

		
		rem 对有 K-Lite Codec Pack的特判
		rem 从这里开始，文档\TestForCheckingKLite.bat处于可用状态
		rem 处理上面的缩进问题
		::set "REG_PATH=HKLM\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\KLiteCodecPack_is1"
::
		::reg query "%REG_PATH%" >nul 2>&1
		::if errorlevel 0 (
		::	echo 检测到 K-Lite Codec Pack。
		::	choice /c yn /m "是否取消部分“无法识别所选择的文件的扩展名”弹窗？"
		::	
		::	exit /b 1
		::)
		echo 所有文件关联配置完成。
    )
    
	echo 所有操作已完成。现在可以尝试启动 Windows Media Player 了。
    pause
)

:end
exit

:SetAssociation
:: 参数1：扩展名（含点），参数2：Progid，参数3：起始位置
set "ext=%~1"
set "progid=%~2"
set "BASE=%~3"
:: 确保扩展名为小写（仅转换字母部分）
for %%a in (a b c d e f g h i j k l m n o p q r s t u v w x y z) do (
    set "ext=!ext:%%a=%%a!"
)
set "key=%BASE%\!ext!\UserChoice"
reg add "!key!" /v "Progid" /t REG_SZ /d "%progid%" /f

:SetNoPromptForExt
set "ext=%~1"
:: 将扩展名转换为小写（兼容性处理）
for %%a in (a b c d e f g h i j k l m n o p q r s t u v w x y z) do (
    set "ext=!ext:%%a=%%a!"
)
set "key=HKCU\Software\Microsoft\MediaPlayer\Player\Extensions\!ext!"
reg add "!key!" /v "Permissions" /t REG_DWORD /d 1 /f
reg add "!key!" /v "Runtime"   /t REG_DWORD /d 1 /f