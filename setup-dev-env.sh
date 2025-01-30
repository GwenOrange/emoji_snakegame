#!/bin/bash

# Enable strict mode
set -e

# Detect Operating System
detect_os() {
    if command -v dnf > /dev/null 2>&1; then
        echo "redhat"
    elif command -v apt-get > /dev/null 2>&1; then
        echo "debian"
    else
        echo "unsupported"
    fi
}

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root (use 'su -' or 'pkexec bash ./script.sh')"
    exit 1
fi

# Install dependencies
install_dependencies() {
    local os=$(detect_os)
    
    case "$os" in
        "debian")
            apt-get update
            apt-get install -y -o Dpkg::Options::="--force-confnew" \
                build-essential \
                cmake \
                gcc \
                g++ \
                gdb \
                libc6-dbg \
                lldb \
                autoconf \
                doctest-dev \
                libtool \
                fakeroot \
                libavdevice-dev \
                libdeflate-dev \
                libgpm-dev \
                libncurses-dev \
                libqrcodegen-dev \
                libswscale-dev \
                libunistring-dev \
                libtinfo-dev \
                ncurses-bin \
                pandoc \
                pkg-config \
                libsdl2-dev \
                libsdl2-mixer-dev
            ;;
            "redhat")
                dnf install -y @development-tools \
                    cmake gcc gcc-c++ gdb lldb autoconf libtool fakeroot \
                    ffmpeg libdeflate-devel gpm-devel ncurses-devel \
                    libunistring-devel pandoc pkgconfig SDL2-devel SDL2_mixer-devel \
                    notcurses-devel notcurses  libqrcodegen-devel  ncurses-devel
                ;;*)
            echo "Unsupported operating system"
            exit 1
            ;;
    esac
}

# Create temporary directory
mkdir -p ./dev_libs_temp
cd ./dev_libs_temp

# Install dependencies
install_dependencies

if [ "$(detect_os)" = "debian" ]; then
    wget https://github.com/dankamongmen/notcurses/archive/refs/tags/v3.0.13.tar.gz -O notcurses.tar.gz
    tar -xzvf notcurses.tar.gz
    cd notcurses-3.0.13
    mkdir -p build
    cd build
    cmake .. -DUSE_GPM=on
    make
    make install
    cd ../..
fi

# Clean up temporary files
cd ..
rm -rf ./dev_libs_temp

echo "All development libraries have been installed!"
