FROM gcc:4.9
RUN apt update || true
RUN apt update || true
RUN apt install -y gdb
