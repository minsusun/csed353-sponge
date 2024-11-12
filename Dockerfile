# Use 20.04 to use clang-format-6.0
FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive
ARG USERNAME=csed353
ARG USER_UID=1000
ARG USER_GID=$USER_UID

ENV TZ=Aisa/Seoul

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME
RUN apt-get update
RUN apt-get install -y \
    sudo \
    build-essential \
    cmake \
    clang-format-6.0 \
    doxygen \
    iproute2 \
    iptables \
    libpcap-dev
RUN echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

USER $USERNAME
WORKDIR /workspace
CMD ["bash", "-l"]
