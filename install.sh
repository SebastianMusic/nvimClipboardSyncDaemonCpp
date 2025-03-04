# remove directory if it already exists
rm -rf /tmp/nvimClipboardSyncBuild/
# Clone the necessary repositories
git clone https://github.com/SebastianMusic/nvimClipboardSyncDaemonCpp.git /tmp/nvimClipboardSyncBuild && \
git clone https://github.com/microsoft/vcpkg /tmp/nvimClipboardSyncBuild/vcpkg && \

# Install vcpkg in the current directory
cd /tmp/nvimClipboardSyncBuild && \
/tmp/nvimClipboardSyncBuild/vcpkg/bootstrap-vcpkg.sh && \
/tmp/nvimClipboardSyncBuild/vcpkg/vcpkg install && \

# Build the daemon
mkdir /tmp/nvimClipboardSyncBuild/build && \
cd /tmp/nvimClipboardSyncBuild/build && \
cmake .. && \
make
echo "to complete the installtion, move the binary from /tmp/nvimClipboardSyncBuild/build/ to any directory in your path"
