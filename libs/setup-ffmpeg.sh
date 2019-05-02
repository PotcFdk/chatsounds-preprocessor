#!/bin/bash
echo "Downloading ffmpeg..."
wget --no-clobber https://ffmpeg.org/releases/ffmpeg-4.1.3.tar.xz
echo "Extracting ffmpeg..."
tar xvf ffmpeg-4.1.3.tar.xz
cd ffmpeg-4.1.3
echo "Configuring ffmpeg (this might take a while)..."
./configure --disable-all --disable-network --disable-autodetect --enable-protocol=file --enable-libmp3lame --enable-avformat --enable-avcodec --enable-demuxer={mp3,ogg,wav} --enable-decoder={vorbis,mp3,pcm_s16le} --enable-static --disable-shared --disable-debug --enable-stripping --prefix="../ffmpeg"
echo "Building ffmpeg..."
mkdir ../ffmpeg
make -j && make install
echo "Finished building ffmpeg."

