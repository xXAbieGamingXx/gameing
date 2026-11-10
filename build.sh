rm -rf build/gaming.exe
rm -rf tools/map_tool.exe
CC=gcc CXX=g++ meson setup build --reconfigure -Dc_args="-std=c23 -ggdb"
meson compile -C build && build/gaming
mv build/map_tool tools/
echo "build done, running exe"
