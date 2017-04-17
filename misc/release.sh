#! /bin/sh
# vim:expandtab:shiftwidth=2:tabstop=8

# Shell script to create "nvwa-x.y.tar.gz"

VER="1.1"
RELEASE="nvwa-$VER"
RELEASE_DIR="$PWD/$RELEASE"
rm -f $RELEASE.tar.gz $RELEASE.zip
rm -rf $RELEASE
mkdir $RELEASE

cd ..
cp -p ChangeLog LICENCE README $RELEASE_DIR

mkdir "$RELEASE_DIR/nvwa"
( cd "nvwa"; cp -p *.cpp *.h "$RELEASE_DIR/nvwa" )

mkdir "$RELEASE_DIR/test"
( cd "test"; cp -p Makefile *.cpp "$RELEASE_DIR/test" )

mkdir "$RELEASE_DIR/doc"
cp -pr doc/html "$RELEASE_DIR/doc/html"
( cd "$RELEASE_DIR/doc/html"; rm -f *.map *.md5 )

cd misc
tar cvfz $RELEASE.tar.gz $RELEASE/

FILES=`find $RELEASE -type f`
for FILE in $FILES; do
  unix2dos -k -s $FILE
done
zip -r $RELEASE.zip $RELEASE/
