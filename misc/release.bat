@echo off

REM Batch file to create "nvwa-x.y.zip"

set VER=0.6
set RELEASE=nvwa-%VER%
rm -rf %RELEASE% %RELEASE%.zip
mkdir %RELEASE%
cd %RELEASE%

set CVSROOT=:pserver:anonymous@cvs.sourceforge.net:/cvsroot/nvwa
cvs login
cvs -z9 co doc
cp -pr ..\..\doc\latex\refman.ps ..\..\doc\html\ doc
cvs -z9 co nvwa
cd doc\html
rm -f *.dot *.map *.md5
cd ..\..

cd ..
zip -r %RELEASE%.zip %RELEASE%/
