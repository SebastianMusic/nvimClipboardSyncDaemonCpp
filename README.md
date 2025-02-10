# Nvim clipboard sync daemon

I am recreating the clipboard sync daemon i started on in december in C++ and aim to
create a nvim plugin that will go well with it.

The goal is to be able to sync your system and nvim clipboard registers with
multiple remote machines at the same time.

The planned functionality is you will run the daemon on a given port on your
local machine. 
```bash
# starts daemon on local machine listening on port 3000
daemon -p 3000 --isLocalMachine
```

Then you can ssh into remote machines with a reverse tunnel
```bash
# forwards localhost:2000 on the remote machine to localhost:3000 on your local
# machine, ergo your daemon
ssh -R 2000:localhost:3000
```
then you will simply start neovim and anything copied will be transfered to
your remote machines clipboard and vice verca. 

I will also have to implement a config file where users can specify a clipboard
command such that it will sync with their local machine. such as for example
`pbcopy` for mac `wl-copy` for linux machines using wayland.



