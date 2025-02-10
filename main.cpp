#include <argparse/argparse.hpp>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <uv.h>

// hopefully unique enough tmp directory
#define TMP_DIR "/tmp/com.sebastianmusic.nvimclipboardsync"
void createTmpDir() {
  struct stat st;
  if (stat(TMP_DIR, &st) == 0) {
    if (!S_ISDIR(st.st_mode)) {
      std::cerr << "Error: " TMP_DIR
                   " exists but is not a directory. Please remove it.\n";
      exit(1);
    }
    std::cout << "Temporary directory already exists: " << TMP_DIR << std::endl;
  } else {
    if (mkdir(TMP_DIR, 0700) != 0) {
      perror("Failed to create tmp directory");
      exit(1);
    }
    std::cout << "Created temporary directory: " << TMP_DIR << std::endl;
  }
}

int main(int argc, char *argv[]) {

  argparse::ArgumentParser program("Nvim Clipboard sync");

  program.add_argument("-p")
      .help("specify the port number to be used, default value is 8080")
      .default_value(8080)
      .scan<'i', int>()
      .nargs(1);

  program.add_argument("--isLocalMachine")
      .help("Specify this argument on your local machine and do not specify it "
            "on your remote servers")
      .default_value(false)
      .implicit_value(true);

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    std::cerr << "Error: " << err.what() << std::endl;
    return 1;
  }

  int port = program.get<int>("-p");
  bool isLocalMachine = program["--isLocalMachine"] == true;
  printf("port is %d\n", port);
  if (isLocalMachine == true) {
    printf("Running on local machine\n");
  } else {
    printf("Running on remote machine\n");
  }

  // Set up tmp directory for local nvim communication

  // main difference between localmachine and !localmachine is that local
  // machine stores an array of remote connections and iterates over them when
  // sending outgoing messages, where as !localmachine stores exactly one
  // remote connection and only forwards messages to that connection
  uv_loop_t *loop = (uv_loop_t *)malloc(sizeof(uv_loop_t));
  uv_loop_init(loop);

  if (isLocalMachine) {
    // set up event listener that listens to new sockets in the TMP dir (
    // sockets are created by the nvim companion plugin and are named using
    // uuid) handler that adds new sockets to a datastructure and starts reading
    // them

    // set up event listener that listens for the removal of sockets in the TMP
    // dir handler that removes the removed socket form the data structure

    // set up event listener that checks if nvim sockets are still used
    // if socket is no longer associated with a process remove it

    // set up event listener that listens for new tcp connections
    // set up handler that adds new tcp connection fd to data structure

    // set up event listener that listens for tcp disconnections
    // set up handler that removes tcp connection fd from data structure

    // set up event listener that listens to new data inside of any nvim sockets
    // Handler for sending this message out to all connected tcp connections

    // set up event listener that listens for new data from tcp connection
    // iterates over nvim connection data structure and sends data to every
    // single one

    // event listener that listens for interrupt signal
    // handler sends interrupt signal to all nvim instances, sends interrupt to
    // all remote servers, and cleans up tmp directory, and stops uv

    // start uv loop
    uv_run(loop, UV_RUN_DEFAULT);
  }
  // main differnece is that !isLocalMachine does not forward interrupt signals
  // sent to it to other tcp connections and it does not iterate nvim messages
  // to multiple tcp connections like isLocalMachine does
  if (!isLocalMachine) {
    // set up event listener that listens to new sockets in the TMP dir (
    // sockets are created by the nvim companion plugin and are named using
    // uuid) handler that adds new sockets to a datastructure and starts reading
    // them

    // set up event listener that listens for the removal of sockets in the TMP
    // dir handler that removes the removed socket form the data structure

    // set up event listener that checks if nvim sockets are still used
    // if socket is no longer associated with a process remove it

    // set up event listener that listens for new tcp connections
    // set up handler that adds new tcp connection fd to data structure

    // set up event listener that listens for tcp disconnections
    // set up handler that removes tcp connection fd from data structure

    // set up event listener that listens to new data inside of any nvim sockets
    // Handler for sending this message out to all connected tcp connections

    // set up event listener that listens for new data from tcp connection
    // iterates over nvim connection data structure and sends data to every
    // single one

    // event listener that listens for interrupt signal handler sends interrupt
    // signal to all nvim instances, and cleans up tmp directory, and stops uv

    // start uv loop
    uv_run(loop, UV_RUN_DEFAULT);
  }

  // Psuedo logic
  //  Daemon sets up a tmp directory which the nvim clients can add a socket to.
  //  daemon listens for new sockets in the directory and adds them to a
  //  datastructure
  //
  // if daemon runs in remote mode try to connect to client on specified port
  // setup event listener to forward incoming messages to all nvim sockets in
  // in tmp directory

  // setup eventlistener to forward outgoing messages from nvim socket directory
  // to localmachine

  // if daemon runs in local mode listen for remote connection run an accept
  // loop and add tcp connections to a datastructure. when reciving incoming
  // messages forward to all remote connections and nvim clients. when getting
  // outgoing nvim messages send to all remote connections

  //  list daemon listens to every socket and if there is some info to be read
  return 0;
}
