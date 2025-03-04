# Nvim clipboard sync daemon
Sync Neovim Clipboards between local and remote machines over ssh reverse tunnels

## Known dependencies 
zip

## Installation Recipe (Currently only for linux)
```bash 
# clone needed repos
git clone https://github.com/SebastianMusic/nvimClipboardSyncDaemonCpp.git /tmp/nvimClipboardSyncBuild && \
git clone https://github.com/microsoft/vcpkg /tmp/nvimClipboardSyncBuild/vcpkg && \
# Install vcpkg in current dir
/tmp/nvimClipboardSyncBuild/vcpkg/bootstrap-vcpkg.sh && \
cd /tmp/nvimClipboardSyncBuild &&  \ 
/tmp/nvimClipboardSyncBuild/vcpkg/vcpkg install && \ 
# Build daemon
mkdir /tmp/nvimClipboardSyncBuild/build && \
cd /tmp/nvimClipboardSyncBuild/build && \
cmake .. && \ 
make 

# Setup config file
mkdir -p ~/.config/nvimClipboardSync/
echo 'copyCmd = ""' > ~/.config/nvimClipboardSync/config.toml
```

#### After installation
You should then move the new binary from the build folder into some folder you
have in your path. such as a programs folder in your home directory
if you do not have such a folder you could make it with
```bash
mkdir ~/myPrograms
mv /tmp/nvimClipboardSyncBuild/build/nvimClipboardSync ~/myPrograms
```
Then you should add this directory to your path
```bash
# for bash
echo 'export PATH="$PATH:/home/<yourUsername>/myPrograms"' > .bashrc
# for zsh
echo 'export PATH="$PATH:/home/<yourUsername>/myPrograms"' > .zshrc
```
## Configuration
After installing the binary the only configuration you should do is provide the
command to use for system clipboard syncing. on mac this would be `pbcopy` and
on wayland it would likely be `wl-copy` but any command in which you could use
it as  `echo "something to copy" | yourCopyCommand` should work
```
nvim ~/.config/nvimClipboardSync/config.toml
```

# How to use
To use the daemon first start it on your localmachine on a port of your choosing
with the --isLocalMachine flag
```bash
# starts daemon on local machine listening on port 3000
./nvimClipboardSync -p 3000 --isLocalMachine &
```

Then next ssh into your remote machine specifying with the -R flag which port on
the remote machine will be forwarded to which port on your local machine;
```bash
# forwards any request made to localhost:2000 on the remote machine to localhost:3000 on your local
# machine, in this case in your daemon.
ssh -R 2000:localhost:3000
```
On the remote machine start the daemon on the port you specifed in the previous
command
```bash
# only specify port on the remote machine
./nvimClipboardSync -p 2000 &
```

**Make sure to have installed the [companion plugin](https://github.com/SebastianMusic/nvimClipboardSyncPlugin) for neovim**

After making sure you have ran the daemon on both machines. To test it out try the following:
1. open a neovim session on both machines.
2. yank some text on your local machine and see how it is transported into your
   remote machines `"0` register.
3. yank some text on your remote machine and see how its both in your system
   clipboard (if you specified a clipboard command) and in the `"0` register on
   your local machine.

# Current limitations
Currently it is only possible to sync from one local machine to one remote
machine but this is being worked on.

# Future todos
Streamline and make installation easier for multiple operating systems
Add possibility to sync multiple remote machines
add correct licensing
