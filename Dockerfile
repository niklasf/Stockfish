FROM debian:wheezy
RUN echo "deb http://archive.debian.org/debian/ wheezy main contrib" > /etc/apt/sources.list && apt-get update --fix-missing && apt-get install -y make g++
VOLUME /home/builder/Stockfish
WORKDIR /home/builder/Stockfish
RUN groupadd -r builder && useradd -r -g builder builder
USER builder
CMD ./linux_build.sh
