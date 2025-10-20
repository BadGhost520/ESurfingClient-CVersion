#!/bin/bash
echo "Go to esurfing dir"
cd esurfing
echo "CMake configuring"
cmake .
echo "Making Programme"
make -j$(nproc)
echo "Moved ESurfingClient to bash dir"
mv bin/ESurfingClient ../ESurfingClient-1.0.0
echo "Protect1"
strip --strip-all ../ESurfingClient-1.0.0
echo "Protect2"
upx --best ../ESurfingClient-1.0.0
