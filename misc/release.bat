@echo off

REM Batch file to create "nvwa-x.y.zip"

set VER=0.8.2
set RELEASE=nvwa-%VER%
rm -rf %RELEASE% %RELEASE%.zip
mkdir %RELEASE%
cd %RELEASE%

set CVSROOT=:pserver:anonymous@nvwa.cvs.sourceforge.net:/cvsroot/nvwa
echo.|cvs login
cvs -z9 co doc nvwa
cp -pr ..\..\doc\latex\refman.pdf ..\..\doc\html doc
cd doc\html
rm -f *.dot *.map *.md5
cd ..\..

cd ..
zip -r %RELEASE%.zip %RELEASE%/
