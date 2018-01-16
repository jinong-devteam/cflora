#!/bin/bash

TARGET_DIR="/usr/local/farmos"
SOURCE_DIR=".."
MODE="release"
if [[ $# -eq 1 ]] ; then
	echo "Mode changed to $1"
	MODE="$1"
fi

rm -rf $TARGET_DIR

mkdir $TARGET_DIR
mkdir "$TARGET_DIR/bin"
mkdir "$TARGET_DIR/conf"
mkdir /var/log/farmos

cp "$SOURCE_DIR/scripts/farmos_gos" "$TARGET_DIR/bin/"
chmod +x "$TARGET_DIR/bin/farmos_gos"
cp "$SOURCE_DIR/scripts/farmos_gcg" "$TARGET_DIR/bin/"
chmod +x "$TARGET_DIR/bin/farmos_gcg"

update-rc.d -f farmos_gos remove
update-rc.d -f farmos_gcg remove
rm /etc/init.d/farmos_gos
rm /etc/init.d/farmos_gcg
ln -s "$TARGET_DIR/bin/farmos_gos" /etc/init.d/farmos_gos
ln -s "$TARGET_DIR/bin/farmos_gcg" /etc/init.d/farmos_gcg
update-rc.d farmos_gos defaults
update-rc.d farmos_gcg defaults

cp "$SOURCE_DIR/gos/$MODE/bin/gos" "$TARGET_DIR/bin/"
cp "$SOURCE_DIR/gcg/$MODE/bin/gcg" "$TARGET_DIR/bin/"
cp "$SOURCE_DIR/conf/farmos-server.ini" "$TARGET_DIR/conf/"


