@echo off

cd ..\Bin\x86\Release\Convolution

cd ..\UDWT

@call run.bat
FOR /L %%i IN (0,1,63) DO (
		echo =====
		echo  %%i
		echo =====
		UDWT.exe %%i 1 ae c
		move "Output\Report2 %%i 0.txt" "Output\Report4 %%i 0 1 c.txt"
	)
)

cd ..\..\..\..\Tests
