#!/bin/bash
echo "Installing dependencies for Foccuss..."

# Detect the Linux distribution
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$NAME
    VER=$VERSION_ID
elif type lsb_release >/dev/null 2>&1; then
    OS=$(lsb_release -si)
    VER=$(lsb_release -sr)
elif [ -f /etc/lsb-release ]; then
    . /etc/lsb-release
    OS=$DISTRIB_ID
    VER=$DISTRIB_RELEASE
elif [ -f /etc/debian_version ]; then
    OS=Debian
    VER=$(cat /etc/debian_version)
else
    OS=$(uname -s)
    VER=$(uname -r)
fi

echo "Detected OS: $OS $VER"

# Install dependencies based on the distribution
case "$OS" in
    "Ubuntu"*|"Debian"*)
        echo "Installing dependencies for Ubuntu/Debian..."
        sudo apt update
        sudo apt install -y \
            build-essential \
            cmake \
            qt6-base-dev \
            qt6-tools-dev \
            qt6-tools-dev-tools \
            qt6-declarative-dev \
            libqt6sql6-sqlite \
            libsqlite3-dev \
            libx11-dev \
            libxext-dev \
            libxcomposite-dev \
            libxrender-dev \
            libxfixes-dev \
            libxdamage-dev \
            libxrandr-dev \
            libxinerama-dev \
            libxcursor-dev \
            libxi-dev \
            libxtst-dev \
            libxss-dev \
            libxcb1-dev \
            libxcb-icccm4-dev \
            libxcb-image0-dev \
            libxcb-keysyms1-dev \
            libxcb-randr0-dev \
            libxcb-render-util0-dev \
            libxcb-shape0-dev \
            libxcb-sync-dev \
            libxcb-xfixes0-dev \
            libxcb-xinerama0-dev \
            libxcb-xkb-dev \
            libxcb-xrm-dev \
            libxcb-xtest0-dev \
            xdotool \
            wmctrl \
            pkg-config
        ;;
    "Fedora"*|"CentOS"*|"Red Hat"*)
        echo "Installing dependencies for Fedora/CentOS/RHEL..."
        sudo dnf install -y \
            gcc-c++ \
            cmake \
            qt6-qtbase-devel \
            qt6-qttools-devel \
            qt6-qtdeclarative-devel \
            qt6-qtnetwork-devel \
            sqlite-devel \
            libX11-devel \
            libXext-devel \
            libXcomposite-devel \
            libXrender-devel \
            libXfixes-devel \
            libXdamage-devel \
            libXrandr-devel \
            libXinerama-devel \
            libXcursor-devel \
            libXi-devel \
            libXtst-devel \
            libXScrnSaver-devel \
            libxcb-devel \
            xdotool \
            wmctrl \
            pkgconfig
        ;;
    "Arch Linux"*)
        echo "Installing dependencies for Arch Linux..."
        sudo pacman -S --needed \
            base-devel \
            cmake \
            qt6-base \
            qt6-tools \
            qt6-declarative \
            qt6-network \
            sqlite \
            libx11 \
            libxext \
            libxcomposite \
            libxrender \
            libxfixes \
            libxdamage \
            libxrandr \
            libxinerama \
            libxcursor \
            libxi \
            libxtst \
            libxss \
            libxcb \
            xdotool \
            wmctrl \
            pkgconf
        ;;
    "openSUSE"*)
        echo "Installing dependencies for openSUSE..."
        sudo zypper install -y \
            gcc-c++ \
            cmake \
            qt6-base-devel \
            qt6-tools-devel \
            qt6-declarative-devel \
            sqlite3-devel \
            libX11-devel \
            libXext-devel \
            libXcomposite-devel \
            libXrender-devel \
            libXfixes-devel \
            libXdamage-devel \
            libXrandr-devel \
            libXinerama-devel \
            libXcursor-devel \
            libXi-devel \
            libXtst-devel \
            libXScrnSaver-devel \
            libxcb-devel \
            xdotool \
            wmctrl \
            pkg-config
        ;;
    *)
        echo "Unsupported distribution: $OS"
        echo "Please install the following packages manually:"
        echo "- build-essential/gcc-c++"
        echo "- cmake"
        echo "- Qt6 development packages (base, tools, declarative, network)"
        echo "- SQLite3 development packages"
        echo "- X11 development packages"
        echo "- XCB development packages"
        echo "- xdotool"
        echo "- wmctrl"
        echo "- pkg-config"
        exit 1
        ;;
esac

echo "Dependencies installed successfully!"
echo "You can now run ./build.sh to build the application." 