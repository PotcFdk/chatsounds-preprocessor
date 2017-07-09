#!/bin/bash
echo "Downloading ffmpeg..."
wget --no-clobber https://ffmpeg.org/releases/ffmpeg-3.3.1.tar.xz
echo "Extracting ffmpeg..."
tar xvf ffmpeg-3.3.1.tar.xz
cd ffmpeg-3.3.1
echo "Patching ffmpeg for our uses..."
patch < ../patches/ffmpeg-configure-no-autodetect-externals.patch
echo "Configuring ffmpeg (this might take a while)..."
./configure --disable-all --enable-libmp3lame --enable-avformat --enable-avcodec --enable-demuxer={mp3,ogg,wav} --enable-decoder={vorbis,mp3,pcm_s16le} --enable-static --disable-shared --disable-debug --enable-stripping --prefix="../ffmpeg"
echo "Building ffmpeg..."
mkdir ../ffmpeg
make -j && make install
echo "Finished building ffmpeg."

