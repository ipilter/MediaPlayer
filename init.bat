@echo off

if exist x64\Debug\default_style.qss (
	del x64\Debug\default_style.qss
)

if exist x64\Release\default_style.qss (
	del x64\Release\default_style.qss
)


mklink /H x64\Debug\default_style.qss .\MediaPlayer\default_style.qss
mklink /H x64\Release\default_style.qss .\MediaPlayer\default_style.qss
pause