#!/bin/bash
echo "Downloading ffmpeg..."
wget --no-clobber https://ffmpeg.org/releases/ffmpeg-4.4.tar.xz
echo "Extracting ffmpeg..."
tar xvf ffmpeg-4.4.tar.xz
cd ffmpeg-4.4
echo "Configuring ffmpeg (this might take a while)..."
sed -i 's/BCRYPT_RNG_ALGORITHM/BCRYPT_NO_RNG_ALGORITHM/g' configure
./configure --arch=x86 --target-os=mingw32 --cross-prefix=i686-w64-mingw32- --disable-all --disable-network --disable-autodetect --enable-protocol=file --enable-libmp3lame --enable-avformat --enable-avcodec --enable-demuxer={mp3,ogg,wav} --enable-decoder={vorbis,mp3,pcm_s16le} --enable-static --disable-shared --disable-debug --enable-stripping --prefix="../ffmpeg"
echo "Building ffmpeg..."
mkdir ../ffmpeg
make -j && make install
echo "Finished building ffmpeg."

