FROM m0rf30/arch-yay

USER user

RUN yay -Sy
RUN yay -S --noconfirm --needed base-devel boost cmake wget
RUN yay -S --noconfirm --needed lame yasm

WORKDIR /preprocessor/
COPY --chown=user:user .git /.git

COPY --chown=user:user libs/setup-ffmpeg-native.sh ./libs/
WORKDIR /preprocessor/libs/
RUN ./setup-ffmpeg-native.sh
WORKDIR /preprocessor/

COPY --chown=user:user cmake ./cmake
COPY --chown=user:user res ./res
COPY --chown=user:user *.hpp* CMakeLists.txt *.cpp ./

USER root
RUN mkdir build
RUN chown -R user:user /preprocessor

USER user
WORKDIR /preprocessor/build/

RUN cmake ..
RUN make

ENTRYPOINT [ "/preprocessor/build/chatsounds-preprocessor" ]

