#!/bin/bash
set -e

: '
SYNOPSIS
    build_install_lib.sh [OPTIONS]

DESCRIPTION
    This script installs VCPKG, fetches a library from GitHub, builds it, and installs it.

OPTIONS
    --vcpkg-dir DIR      Set the installation directory for VCPKG (default: $HOME/vcpkg)
    --lib-dir DIR        Set the directory for library installation (default: $HOME/libs)
    --tag TAG            Specify the branch or tag to checkout (default: latest release)
    --install-path DIR   Set the installation directory for Sensing-Dev (ion-kit)
'
# DEFAULT ######################################################################
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKING_DIR="$SCRIPT_DIR/cuda-setup"
VCPKG_DIR="$WORKING_DIR/vcpkg"
GITHUB_REPO="fixstars/ion-kit"
BRANCH_OR_TAG="latest"
SDK_DIR="/opt/sensing-dev"


# VCPKG ########################################################################
while [[ "$#" -gt 0 ]]; do
    case "$1" in
        --vcpkg-dir)
            VCPKG_DIR="$2"
            shift 2
            ;;
        --lib-dir)
            LIBRARY_DIR="$2"
            shift 2
            ;;
        --version)
            BRANCH_OR_TAG="$2"
            shift 2
            ;;
        --install-path)
            SDK_DIR="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

if [[ "$BRANCH_OR_TAG" =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]] || [[ "$BRANCH_OR_TAG" == "LATEST" ]]; then
    echo "Valid --version: $BRANCH_OR_TAG"
else
    echo "Error: --version must be in the format 'vX.Y.Z' (e.g., v1.8.10)"
    exit 1
fi

# Get git ######################################################################
sudo apt install git-all -y

# Set working directory ########################################################
mkdir -p "$WORKING_DIR"

# VCPKG ########################################################################
if [ ! -d "$VCPKG_DIR" ]; then
    echo "Cloning VCPKG..."
    git clone https://github.com/microsoft/vcpkg.git "$VCPKG_DIR"
    cd "$VCPKG_DIR" || exit
    ./bootstrap-vcpkg.sh
else
    echo "VCPKG already installed in $VCPKG_DIR"
fi
export PATH=$VCPKG_DIR:$PATH

# Get Halide ###################################################################
cd "$WORKING_DIR"
echo $BRANCH_OR_TAG
if [[ "$BRANCH_OR_TAG" =~ ^v1\.8\.[0-9]+$ ]]; then
    # v1.8.xx の場合
    curl -L https://github.com/halide/Halide/releases/download/v16.0.0/Halide-16.0.0-x86-64-linux-1e963ff817ef0968cc25d811a25a7350c8953ee6.tar.gz --output halide.tar.gz
    tar -xvzf halide.tar.gz
    cd Halide-16.0.0-x86-64-linux
    export HALIDE_PATH="$WORKING_DIR/Halide-16.0.0-x86-64-linux"
else
    # それ以外の場合（デフォルト）
    curl -L https://github.com/halide/Halide/releases/download/v17.0.1/Halide-17.0.1-x86-64-linux-52541176253e74467dabc42eeee63d9a62c199f6.tar.gz --output halide.tar.gz
    tar -xvzf halide.tar.gz
    cd Halide-17.0.1-x86-64-linux
    export HALIDE_PATH="$WORKING_DIR/Halide-17.0.1-x86-64-linux"
fi

# Get ion-kit ##################################################################
cd "$WORKING_DIR" || exit
echo "Fetching library from GitHub..."
if [ "$BRANCH_OR_TAG" = "latest" ]; then
    LATEST_TAG=$(curl -s "https://api.github.com/repos/$GITHUB_REPO/releases/latest" | grep 'tag_name' | cut -d '"' -f 4)
    BRANCH_OR_TAG="$LATEST_TAG"
fi
echo "ion-kit $BRANCH_OR_TAG will be installed..."
if [ -d "$WORKING_DIR/ion-kit" ]; then
    rm -r "$WORKING_DIR/ion-kit"
fi

git clone --branch "$BRANCH_OR_TAG" --depth 1 "https://github.com/$GITHUB_REPO.git"


# Build and Install ############################################################
cd "$WORKING_DIR/ion-kit"
# Set minimum CMake 3.22
CMAKE_FILE="$WORKING_DIR/ion-kit/CMakeLists.txt"
if [ -f "$CMAKE_FILE" ]; then
    sed -i 's/cmake_minimum_required(VERSION 3\.25)/cmake_minimum_required(VERSION 3.22)/' "$CMAKE_FILE"
    echo "Updated cmake_minimum_required version in CMakeLists.txt"
fi
# Set up dependencies
vcpkg install
# Build
mkdir -p build && cd build
cmake -D CMAKE_TOOLCHAIN_FILE=${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake \
-D Halide_DIR=${HALIDE_PATH}/lib/cmake/Halide \
-D HalideHelpers_DIR=${HALIDE_PATH}/lib/cmake/HalideHelpers \
-DCMAKE_BUILD_TYPE=Release -D ION_BUILD_TEST=OFF -D BUILD_DOC=OFF -D ION_BUILD_EXAMPLE=OFF \
-D CMAKE_INSTALL_PREFIX=${SDK_DIR} ..
# Install
cmake --build . --target install

echo "Library installation completed."

VERSION_INFO_FILE="$SDK_DIR/version_info.json"
if [ -f "$VERSION_INFO_FILE" ]; then
    sed -i "s/\"ion-kit\": \"v[0-9]*\\.[0-9]*\\.[0-9]*\"/\"ion-kit\": \"$BRANCH_OR_TAG(CUDA)\"/" "$VERSION_INFO_FILE"
    sed -i "s/\"ion-kit\": \"[0-9]*\\.[0-9]*\\.[0-9]*\"/\"ion-kit\": \"$BRANCH_OR_TAG(CUDA)\"/" "$VERSION_INFO_FILE"
    echo "Updated $SDK_DIR/version_info.json"
fi
