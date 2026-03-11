rm -rf builddir/gaming.exe
CC=gcc CXX=g++ meson setup builddir --reconfigure
meson compile -C builddir
builddir/gaming
