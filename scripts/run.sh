#!/bin/bash
# Simple run script for testing the bootloader

cd "$(dirname "$0")/.."

echo "Building bootloader..."
make clean
make

echo ""
echo "Running in QEMU..."
echo "Press Ctrl+C to exit"
echo ""

make run
