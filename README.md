# Nvim Clipboard Sync Daemon

Sync Neovim clipboards between local and remote machines over SSH reverse tunnels.

## Known Dependencies

- `zip`

## Installation (Linux Only)

```bash
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
```

### Post-Installation

To make the binary easily executable, move it to a directory in your PATH:

```bash
mkdir -p ~/.local/bin
mv /tmp/nvimClipboardSyncBuild/build/nvimClipboardSync ~/.local/bin/
```

Ensure the directory is included in your PATH:

```bash
# For Bash users
echo 'export PATH="$PATH:$HOME/.local/bin"' >> ~/.bashrc

# For Zsh users
echo 'export PATH="$PATH:$HOME/.local/bin"' >> ~/.zshrc
```

### Configuration

Edit the configuration file to specify the command for system clipboard syncing. Common commands are `pbcopy` for macOS and `wl-copy` for Wayland. Ensure the command can handle input piped via stdin (e.g., `echo "text" | yourCopyCommand`).

```bash
nvim ~/.config/nvimClipboardSync/config.toml
```

## Usage

1. Start the daemon on your local machine, specifying a port and the `--isLocalMachine` flag:

    ```bash
    # Start the daemon on the local machine listening on port 3000
    ./nvimClipboardSync -p 3000 --isLocalMachine &
    ```

2. SSH into your remote machine, setting up port forwarding:

    ```bash
    # Forward requests from localhost:2000 on the remote machine to localhost:3000 on the local machine
    ssh -R 2000:localhost:3000 user@remote-machine
    ```

3. On the remote machine, start the daemon on the specified port:

    ```bash
    # Start the daemon on port 2000 on the remote machine
    ./nvimClipboardSync -p 2000 &
    ```

**Note:** Install the [companion plugin](https://github.com/SebastianMusic/nvimClipboardSyncPlugin) for Neovim.

### Testing

1. Open Neovim on both the local and remote machines.
2. Yank text on your local machine and observe its sync to the `"0` register on the remote machine.
3. Yank text on your remote machine and see it synchronizing both to your system clipboard (if configured) and the `"0` register on your local machine.

## Current Limitations

- Currently supports clipboard syncing between one local machine and one remote machine.

## Future Plans

- Streamline installation across multiple operating systems.
- Enable sync to and from multiple remote machines.
- Add appropriate licensing.
