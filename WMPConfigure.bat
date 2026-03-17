@echo off
cd /d "%~dp0"
setlocal enabledelayedexpansion
rem 进行调优工作，还是进行覆写安装？

:menu
echo [0] 退出程序
echo [1] 执行部署过程
echo [2] 检查依赖项目
echo [3] 查看已知限制
echo [4] 执行多用户部署

choice /c 01234 /m "请选择一个选项"

if errorlevel 5 goto MultiUsers
if errorlevel 4 goto Limits
if errorlevel 3 goto Check
if errorlevel 2 goto Execute
if errorlevel 1 exit

:Limits
echo Windows Media Center 必须被卸载，目前为止没有找到解决方法。
::echo 
pause
goto menu

:Check

ver | find "6.1" > nul
if errorlevel 1 (
    echo Warning: 不是被测试通过的版本
    echo Info: 若选择继续，将导致无法预料的情况
) else (
	echo 系统版本检测： Windows 7 [正确]
)

if not exist "%WinDir%\System32\regsvr32.exe" ( if not exist ".\regsvr32.exe" ( echo Fatal Error: 找不到 regsvr32.exe。无法注册 DLL 文件。请从其他计算机上复制文件到本脚本同级目录下，然后再展开部署。) ) else ( echo regsvr32.exe 存在 )
if not exist "%WinDir%\System32\reg.exe" ( if not exist ".\reg.exe" ( echo Warning: 找不到 reg.exe，需要保证regedit.exe存在并手动导入.reg文件 ) ) else ( echo reg.exe 存在 )
if not exist "wmp9xp" ( echo Error:找不到名为 wmp9xp 的文件夹，请重新下载程序 ) else ( echo wmp9xp 文件夹存在 )
if not exist "wmp" ( echo Error:找不到名为 wmp 的文件夹，请重新下载程序 ) else ( echo wmp 文件夹存在 )
::if not exist "%WinDir%\System32\wmic.exe" ( echo Error: 找不到 wmic。您需要手动在控制面板：打开或关闭 Windows 功能中卸载组件，在本脚本所在目录下创建名为1的文件夹，重新启动计算机，然后再次运行该程序。这样将跳过第一阶段的卸载过程。 ) else ( echo wmic 命令存在 )
if not exist "%WinDir%\System32\xcopy.exe" ( if not exist ".\xcopy.exe" ( echo Warning: 找不到 xcopy.exe，无法复制文件 ) ) else ( echo xcopy.exe 存在 )
if not exist ".\SetOpeningMethod.reg" echo Warning:找不到名为 SetOpeningMethod.reg 的文件夹，需要手动配置注册表项

pause
cls
goto menu

:Execute
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
			wmic.exe /Namespace:\\root\default Path SystemRestore Call CreateRestorePoint "降级至 Windows Media Player 9", 100, 7
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
	choice /c yn /m "现在就重新启动计算机吗？"
	if errorlevel 1 shutdown -r -t 0
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
   
:MultiUsers		   
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
	

	:: 写入所有必需的注册表键值
	reg import SetOpeningMethod.reg
	
	echo 注册表项已添加完成，Windows Media Player 将跳过首次运行配置。
	
    choice /c yn /m "重新注册快捷方式？"
    if errorlevel 2 (
        echo 跳过快捷方式注册
    ) else (
        echo 正在注册快捷方式...
		xcopy ".\Windows Media Player.lnk" "%ProgramData%\Microsoft\Windows\Start Menu\Programs" /E /Y
		
		::注意：Windows XP可以运行程序，但什么也不会发生
		PinToTaskbar.exe > nul

    )
    
    choice /c yn /m "重新注册打开方式？"
    if errorlevel 2 (
        echo 跳过打开方式注册
    ) else (
        echo 正在注册打开方式...
        reg import SetOpeningMethod.reg
		
		
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
	echo 是否重新启动文件资源管理器并刷新缓存？
    pause
)

:end
exit

