FROM debian:oldstable
RUN apt-get update && apt-get install -y make g++
VOLUME /root/Stockfish
WORKDIR /root/Stockfish
CMD ./unix_build.sh
