rm -rf builddir/gaming.exe
CC=gcc CXX=g++ meson setup builddir --reconfigure -Dc_args="-std=c23"
meson compile -C builddir
builddir/gaming
