@echo off
mkdir UltraBin
cd vncviewer
make -fMakefile.bcc32 -s -l
cd ..
copy /b vncviewer\vncviewer.exe ultrabin
cd winvnc
make -fMakefile.bcc32 -s -l
cd ..
copy /b winvnc\VNCHooks\VNCHooks.dll ultrabin
copy /b winvnc\winvnc.exe ultrabin
