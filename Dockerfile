FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install system dependencies
RUN apt update && apt install -y \
	build-essential \
	cmake \
	ninja-build \
	git \
	curl \
	wget \
	unzip \
	tar \
	sudo \
	autoconf \
	automake \
	libtool \
	pkg-config \
	zip \
	neovim \
	&& apt clean

# Clone the project and vcpkg, then bootstrap vcpkg
RUN git clone https://github.com/SebastianMusic/nvimClipboardSyncDaemonCpp.git /root/nvimClipboardSyncDaemonCpp 

RUN	git clone https://github.com/microsoft/vcpkg.git /root/nvimClipboardSyncDaemonCpp/vcpkg 
RUN	/root/nvimClipboardSyncDaemonCpp/vcpkg/bootstrap-vcpkg.sh
