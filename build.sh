rm -rf builddir/gaming.exe
rm -rf tools/map_tool.exe
CC=gcc CXX=g++ meson setup builddir --reconfigure -Dc_args="-std=c23 -g"
meson compile -C builddir
mv builddir/map_tool.exe tools/map_tool.exe
builddir/gaming
