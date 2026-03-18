>[!Caution]
>This tutorial is in test stage and should not be used currently.

## Following files should be copied in Windows XP Professional directly:

Note that you might need a virtual machine(recommended, because the system is completely clean) or an old machine installed with Windows XP. Use language you are familiar with.

| File name    | Full path[^1]                                      |
| ------------ | -------------------------------------------------- |
| devenum.dll  | C:\WINDOWS\system32\devenum.dll                    |
| dxmasf.dll   | C:\WINDOWS\system32\dxmasf.dll                     |
| mplayer2.exe | C:\Program Files\Windows Media Player\mplayer2.exe |
| mplayer2.hlp | C:\WINDOWS\Help\mplayer2.hlp                       |
| msdxm.ocx    | C:\WINDOWS\system32\msdxm.ocx                      |
| msdxmlc.dll  | C:\WINDOWS\system32\msdxmlc.dll                    |
| plyr_err.chm | C:\WINDOWS\Help\plyr_err.chm                       |
| unregmp2.exe | C:\WINDOWS\inf\unregmp2.exe                        |
| winhlp32.hlp | C:\WINDOWS\Help\winhlp32.hlp                       |
| wmp.dll      | C:\WINDOWS\system32\wmp.dll                        |
| wmp.ocx      | C:\WINDOWS\system32\wmp.ocx                        |
| wmpasf.dll   | C:\WINDOWS\system32\wmpasf.dll                     |
| wmpcd.dll    | C:\WINDOWS\system32\wmpcd.dll                      |
| wmpcore.dll  | C:\WINDOWS\system32\wmpcore.dll                    |
| wmpdxm.dll   | C:\WINDOWS\system32\wmpdxm.dll                     |
| wmphoto.dll  | C:\WINDOWS\system32\wmphoto.dll                    |
| wmploc.dll   | C:\WINDOWS\system32\wmploc.dll                     |
| wmpshell.dll | C:\WINDOWS\system32\wmpshell.dll                   |
| wmpui.dll    | C:\WINDOWS\system32\wmpui.dll                      |
| wmsdmod.dll  | C:\WINDOWS\system32\wmsdmod.dll                    |
| wmsdmoe.dll  | C:\WINDOWS\system32\wmsdmoe.dll                    |
| wmsdmoe2.dll | C:\WINDOWS\system32\wmsdmoe2.dll                   |
| wmspdmod.dll | C:\WINDOWS\system32\wmspdmod.dll                   |
| wmspdmoe.dll | C:\WINDOWS\system32\wmspdmoe.dll                   |
| wmstream.dll | C:\WINDOWS\system32\wmstream.dll                   |
| wmv8ds32.ax  | C:\WINDOWS\system32\wmv8ds32.ax                    |
| WMVCore.dll  | C:\WINDOWS\system32\wmvcore.dll                    |
| wmvdmod.dll  | C:\WINDOWS\system32\wmvdmod.dll                    |
| wmvdmoe2.dll | C:\WINDOWS\system32\wmvdmoe2.dll                   |
| wmvds32.ax   | C:\WINDOWS\system32\wmvds32.ax                     |

[^1]: You can also search using Everything for a rapid search, use files in C:\Program Files\Windows Media Player if you can, or C:\WINDOWS\system32 is also considerable.

Then copy all the files in WMP_Config\wmp9xp, replace all the files.

## Get installation pack

#### Preparation for installer

Note: if just change your language, skipping this step will take no effect on ultimate process, and could save dozens of minutes.

Search "Windows Media Player 9 Series" and you are probably to find "Windows Media Player 9 build 2601" in archive.org. This is the installer with original version in Windows XP SP2. Extract wmp9.2601.exe in the iso file directly to an empty folder.

#### Preparation for files in Windows XP

English users could skip for some files.

Because WMP relies some files not in any installers at all, we need to prepare files by overselves. Setup a Windows XP virtual machine or turn on your old machines. Please note that do NOT install updates for WMP, otherwise you might get the version you don't mean to get, WMP 10, for example(Refer to [Windows Media Player 10]). Otherwise the worst condition might be WMP refusing to startup. We suggest NOT to combine files extracted from wmp9.2601.exe and copied from Windows XP.

#### Overwrite files and start installation

The files extracted from wmp9.2601.exe corresponds to the folder wmp9xp. The files copied from Windows XP corresponds to the folder wmp.

[!Note]

This script will not check files in the folder so take your own risk on replacing files or downloading from other sources.
