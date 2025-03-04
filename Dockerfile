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

RUN rm /root/.bashrc
RUN echo "# Enable vi mode for command line editing" >> /root/.bashrc && \
	echo "set -o vi" >> /root/.bashrc && \
	echo "" >> /root/.bashrc && \
	echo "# Improve autocompletion" >> /root/.bashrc && \
	echo "bind 'set completion-ignore-case on'" >> /root/.bashrc && \
	echo "bind 'set show-all-if-ambiguous on'" >> /root/.bashrc && \
	echo "bind 'set menu-complete-display-prefix on'" >> /root/.bashrc && \
	echo "" >> /root/.bashrc && \
	echo "# Enhance the prompt" >> /root/.bashrc && \
	echo "export PS1='\[\e[1;32m\]\u@\h:\w\$ \[\e[0m\]'" >> /root/.bashrc && \
	echo "" >> /root/.bashrc && \
	echo "alias ls='ls --color=auto'" >> /root/.bashrc && \
	echo "alias ll='ls -lh'" >> /root/.bashrc && \
	echo "alias la='ls -lha'" >> /root/.bashrc && \
	echo "" >> /root/.bashrc && \
	echo "# Enable history search with arrow keys (for vi mode)" >> /root/.bashrc && \
	echo "bind '\"\\e[A\":history-search-backward'" >> /root/.bashrc && \
	echo "bind '\"\\e[B\":history-search-forward'" >> /root/.bashrc && \
	echo "" >> /root/.bashrc && \
	echo "# Ignore duplicate commands in history" >> /root/.bashrc && \
	echo "export HISTCONTROL=ignoredups:erasedups" >> /root/.bashrc && \
	echo "" >> /root/.bashrc && \
	echo "# Keep a small history file to avoid bloat in containers" >> /root/.bashrc && \
	echo "export HISTFILE=/dev/null" >> /root/.bashrc && \
	echo "export HISTSIZE=500" >> /root/.bashrc && \
	echo "export SAVEHIST=500" >> /root/.bashrc && \
	echo "" >> /root/.bashrc && \
	echo "# Better defaults for `grep` and `less`" >> /root/.bashrc && \
	echo "alias grep='grep --color=auto'" >> /root/.bashrc && \
	echo "alias n='nvim'" >> /root/.bashrc && \
	echo "export LESS='-R'" >> /root/.bashrc && \
	echo "" >> /root/.bashrc && \
	echo "# Ensure the terminal behaves well in Docker" >> /root/.bashrc && \
	echo "export TERM=xterm-256color" >> /root/.bashrc
