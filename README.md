# WIP INFO
Neovim plugin is currently not correctly packeged so will not work as says in
this readme

# Nvim clipboard sync daemon
Sync Clipboards between local and remote machines over ssh reverse tunnels

## Installation (WIP)

## Configuration
```bash
# Create directory 
mkdir ~/.config/nvimClipboardSync/
~/.config/nvimClipboardSync/
echo 'copyCmd = ""' > config.toml
```
Then open the file and add your clipboard command on make this would be `pbcopy`
and if you are on wayland it might be `wl-copy`
```
nvim ~/.config/nvimClipboardSync/config.toml
```

## How to use
To use the daemon first start it on your localmachine on a port of your choosing
with the --isLocalMachine flag
```bash
# starts daemon on local machine listening on port 3000
./nvimClipboardSync -p 3000 --isLocalMachine &
```

Then next ssh into your remote machine specifying with the -R flag which port on
the remote machine will be forwarded to which port on your local machine;
```bash
# forwards localhost:2000 on the remote machine to localhost:3000 on your local
# machine, ergo your daemon
ssh -R 2000:localhost:3000
```
On the remote machine start the daemon on the port you specifed in the previous
command
```bash
# only specify port on the remote machine
./nvimClipboardSync -p 2000 &
```

Make sure to have installed the [companion plugin](https://github.com/SebastianMusic/nvimClipboardSyncPlugin) for neovim

Now with the daemon running and the neovim plugin installed you can open up neovim to automaticlly connect to your daemon.
simply yank any text and this will be sent to the other machine
When text is sent from the local machine to the remote machine paste it using the
`"0` register

# Current limitations
Currently syncing from one remote machine to another, but it is possible to sync
from many remote machines to the same local machine. This is being worked on

# Future todos
Streamline and make installation easier 
Add possibility to sync multiple remote machines
Better precision timestamps to make yanking in quick succession possible
