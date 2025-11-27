#!/bin/bash

TARGET_DIR="/usr/local/bin"
EXECUTABLE_NAME="k88"

make clean
make

if [ $? -ne 0 ]; then
    exit 1
fi

if [ ! -f "$EXECUTABLE_NAME" ]; then
    exit 1
fi

if [ ! -d "$TARGET_DIR" ]; then
    sudo mkdir -p "$TARGET_DIR"
fi

sudo cp "$EXECUTABLE_NAME" "$TARGET_DIR/"

if [ $? -ne 0 ]; then
    exit 1
fi

sudo chmod +x "$TARGET_DIR/$EXECUTABLE_NAME"

make clean

exit 0