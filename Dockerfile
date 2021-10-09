# build image

FROM alpine:3.12
RUN apk --no-cache add git build-base cmake boost-dev lame-dev yasm

WORKDIR /preprocessor/
COPY .git /.git
COPY cmake ./cmake
COPY res ./res
COPY *.hpp* CMakeLists.txt *.cpp ./
RUN mkdir build

COPY libs/setup-ffmpeg-native.sh ./libs/
WORKDIR /preprocessor/libs/
RUN /bin/sh ./setup-ffmpeg-native.sh

WORKDIR /preprocessor/build/
RUN cmake -DCMAKE_BUILD_TYPE=Release ..
RUN make

RUN mv chatsounds-preprocessor /usr/bin/

# production

FROM alpine:3.12
RUN apk --no-cache add boost
COPY --from=0 /usr/bin/chatsounds-preprocessor /usr/bin
ENTRYPOINT [ "/usr/bin/chatsounds-preprocessor" ]
