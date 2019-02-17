#!/bin/bash

sudo apt-get update
sudo apt-get -y install autoconf automake build-essential libtool intltool git pkg-config texi2html yasm nasm checkinstall \
                libass-dev libfaac-dev libgpac-dev libmp3lame-dev libopus-dev libtheora-dev libvorbis-dev libvpx-dev zlib1g-dev \
                liblzma-dev lib-bz2-dev libvdpau-dev  libcurl4-openssl-dev libxml2-dev libgtk2.0-dev libnotify-dev libglib2.0-dev libevent-dev
                
                
# YASM
wget http://www.tortall.net/projects/yasm/releases/yasm-1.2.0.tar.gz
tar xf yasm-1.2.0.tar.gz
cd yasm-1.2.0
./configure
make
make install
cd ..

# x264
git clone --depth 1 https://git.videolan.org/git/x264.git
cd x264
./configure --enable-static
make
checkinstall
cd ..

# fdk-aac
git clone --depth 1 https://github.com/mstorsjo/fdk-aac.git
cd fdk-aac
autoreconf -fiv
./configure --disable-shared
make
checkinstall
cd ..

# ffmpeg
wget https://www.ffmpeg.org/releases/ffmpeg-4.1.tar.bz2
tar jxvf ffmpeg-4.1.tar.bz2
cd ffmpeg-4.1
./configure  --enable-gpl --enable-libass --enable-libfdk-aac --enable-libmp3lame --enable-libopus --enable-libtheora --enable-libvorbis --enable-libvpx --enable-libx264 --enable-nonfree
make 
checkinstall
cd ..


--prefix=/usr --extra-version='1~deb9u1' --toolchain=hardened --libdir=/usr/lib/x86_64-linux-gnu --incdir=/usr/include/x86_64-linux-gnu --enable-gpl --disable-stripping --enable-avresample --enable-avisynth --enable-gnutls --enable-ladspa --enable-libass --enable-libbluray --enable-libbs2b --enable-libcaca --enable-libcdio --enable-libebur128 --enable-libflite --enable-libfontconfig --enable-libfreetype --enable-libfribidi --enable-libgme --enable-libgsm --enable-libmp3lame --enable-libopenjpeg --enable-libopenmpt --enable-libopus --enable-libpulse --enable-librubberband --enable-libshine --enable-libsnappy --enable-libsoxr --enable-libspeex --enable-libssh --enable-libtheora --enable-libtwolame --enable-libvorbis --enable-libvpx --enable-libwavpack --enable-libwebp --enable-libx265 --enable-libxvid --enable-libzmq --enable-libzvbi --enable-omx --enable-openal --enable-opengl --enable-sdl2 --enable-libdc1394 --enable-libiec61883 --enable-chromaprint --enable-frei0r --enable-libopencv --enable-libx264 --enable-shared