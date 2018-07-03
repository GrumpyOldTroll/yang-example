# Copyright Akamai 2018, MIT-licensed (see LICENSE)
FROM ubuntu:latest
MAINTAINER Jake Holland <jholland@akamai.com>

RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    cmake \
    flex \
    bison \
    libpcre3-dev \
    sudo \
    valgrind \
    gdb \
    vim

RUN useradd -ms /bin/bash yangex
RUN echo "yangex ALL=NOPASSWD: ALL" >> /etc/sudoers
WORKDIR /home/yangex
USER yangex

RUN git clone https://github.com/CESNET/libyang
RUN cd libyang && ( ( cmake -DENABLE_CACHE=OFF . && make && sudo make install ) ; cd .. )
RUN cd libyang/src/extensions && ( ( make && sudo make install ) ; cd ../../.. )

ENV LD_LIBRARY_PATH /usr/local/lib
COPY --chown=yangex:yangex example/* /home/yangex/
RUN ./build.sh
RUN ./yex
ENTRYPOINT ["/bin/bash"]
