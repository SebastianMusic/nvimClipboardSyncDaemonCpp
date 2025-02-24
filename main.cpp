// TODO: Maybe not have a config file on remote machine since we dont need to
// sync clipboards there or atleast have an option to turn clipboard syncing off

#include <argparse/argparse.hpp>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <netinet/in.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <toml++/toml.hpp>
#include <unistd.h>
#include <uv.h>
#include <vector>

/*   __________  _   __________________*/
/*  / ____/ __ \/ | / / ____/  _/ ____/*/
/* / /   / / / /  |/ / /_   / // / __  */
/*/ /___/ /_/ / /|  / __/ _/ // /_/ /  */
/*\____/\____/_/ |_/_/   /___/\____/   */
/*                                     */

void getConfigDir(const std::vector<std::string> configDirectories,
                  std::string &configDir, int &error) {
  for (const std::string &path : configDirectories) {
    if (std::filesystem::exists(path)) {
      configDir = path;
      error = 0;
      return;
    }
  }
  error = -1;
}
/*   __  ______________    ____________  __*/
/*  / / / /_  __/  _/ /   /  _/_  __/\ \/ /*/
/* / / / / / /  / // /    / /  / /    \  / */
/*/ /_/ / / / _/ // /____/ /  / /     / /  */
/*\____/ /_/ /___/_____/___/ /_/     /_/   */
/*                                         */
std::vector<std::string> getFilenamesInDirectory(const std::string &directory) {
  std::vector<std::string> filenames;

  for (const auto &entry : std::filesystem::directory_iterator(directory)) {

    filenames.push_back(entry.path().filename().string());
  }
  return filenames;
}
//    ________    ____  ____  ___    __
//   / ____/ /   / __ \/ __ )/   |  / /
//  / / __/ /   / / / / __  / /| | / /
// / /_/ / /___/ /_/ / /_/ / ___ |/ /___
// \____/_____/\____/_____/_/  |_/_____/
//
//
uv_stream_t *GLOBAL_LOCAL_HANDLE = new uv_stream_t;
uv_loop_t *loop = (uv_loop_t *)malloc(sizeof(uv_loop_t));
bool isLocalMachine;
std::vector<uv_stream_t *> connectedDaemonHandles;

// hopefully unique enough tmp directory
#define TMP_DIR "/tmp/com.sebastianmusic.nvimclipboardsync/"
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

void iterateOverStreams(std::vector<uv_stream_t *> vectorToIterateOver,
                        const uv_buf_t *buf, ssize_t nread) {
  spdlog::info("entered iterateOverStreams");
  if (buf != NULL) {
    spdlog::info("buffer is {}", std::string(buf->base, buf->len));
  } else {
    spdlog::info("buffer is empty");
  }

  for (uv_stream_t *handle : vectorToIterateOver) {

    uv_write_t *writeRequest = new uv_write_t();
    uv_buf_t bufs = uv_buf_init(buf->base, nread);

    int result = uv_write(
        writeRequest, handle, &bufs, 1, [](uv_write_t *req, int status) {
          if (status < 0) {
            std::cerr << "Error in write in daemonToDaemonReadCallback:"
                      << uv_strerror(status) << std::endl;
          }
          delete req;
        });

    if (result < 0) {
      std::cerr << "uv_write failed: " << uv_strerror(result) << std::endl;
      delete writeRequest;
    }
  }
}

//  _   ___     _____ __  __   _____ ___
// | \ | \ \   / /_ _|  \/  | |_   _/ _ \/
// |  \| |\ \ / / | || |\/| |   | || | | |
// | |\  | \ V /  | || |  | |   | || |_| |
// |_| \_|  \_/  |___|_|  |_|   |_| \___/
//
//  ____    _    _____ __  __  ___  _   _
// |  _ \  / \  | ____|  \/  |/ _ \| \ | |
// | | | |/ _ \ |  _| | |\/| | | | |  \| |
// | |_| / ___ \| |___| |  | | |_| | |\  |
// |____/_/   \_\_____|_|  |_|\___/|_| \_|
//

// filesystem structs
// Filesystem functions

static void nvimMemoryAllocationCallback(uv_handle_t *handle,
                                         size_t suggested_size, uv_buf_t *buf) {
  buf->base = (char *)malloc(suggested_size);
  buf->len = suggested_size;
}

void readFromNvim(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
  // check if nread is less than 0 if it is then there is an error
  if (nread < 0) {
    // stop stream on error
    uv_read_stop(stream);
  }
  iterateOverStreams(connectedDaemonHandles, buf, nread);
}

void nvimConnectCallback(uv_connect_t *req, int status) {
  if (status < 0) {
    std::cout << "error occured in connection callback could not connect";
    return;
  }
  uv_read_start(req->handle, nvimMemoryAllocationCallback, readFromNvim);
}
typedef std::map<std::string, uv_stream_t *> PipenameToHandle;

std::vector<PipenameToHandle> connectedNvimClientsPipesMap;
std::vector<uv_stream_t *> connectedNvimClientsPipes;
void tmpDirectoryUpdateEventCallback(uv_fs_event_t *handle,
                                     const char *filename, int events,
                                     int status) {
  // Might have to convert filename to type int?
  if (filename != NULL) {
    if (std::find_if(connectedNvimClientsPipesMap.begin(),
                     connectedNvimClientsPipesMap.end(),
                     [&](const PipenameToHandle &map) {
                       return map.find(filename) != map.end();
                     }) != connectedNvimClientsPipesMap.end()) {

      std::cout << "file descriptor for nvim domain socket already exists in "
                   "vector\n";
    } else {
      // create a pipe type for the new unix domain socket
      uv_pipe_t *newSocket = new uv_pipe_t;
      uv_pipe_init(loop, newSocket, 1);
      uv_connect_t *connectionRequest = new uv_connect_t;
      uv_pipe_connect(connectionRequest, newSocket,
                      (std::string(TMP_DIR) + filename).c_str(),
                      nvimConnectCallback);

      PipenameToHandle newMap;
      newMap[filename] = ((uv_stream_t *)&newSocket);
      connectedNvimClientsPipesMap.push_back(newMap);
      connectedNvimClientsPipes.push_back((uv_stream_t *)&newSocket);
    }
  } else {
    std::cout << "filename is null\n";
  }
}

void cleanTmpDir() {
  // first get all the pipe names in the directory add them to an array
  std::vector<std::string> filenames = getFilenamesInDirectory(TMP_DIR);
  // then create array to store files that need to be closed
  std::vector<std::string> toBeRemoved;
  // make a vector with all filenames from nvimconnectedpipes in correct order
  // for easier lookup
  std::vector<std::string> connectedNvimClientsNames;
  for (int i = 0; i < connectedNvimClientsPipesMap.size(); i++) {
    connectedNvimClientsNames.push_back(
        connectedNvimClientsPipesMap[i].begin()->first);
  }
  // check if there are files in the nvimconnectedpipes vector
  // which are not currently here
  for (const auto filename : filenames) {
    auto it = std::find(connectedNvimClientsNames.begin(),
                        connectedNvimClientsNames.end(), filename);
    // if it does not find the file
    if (it == connectedNvimClientsNames.end()) {
      toBeRemoved.push_back(filename);
    }
  }

  // remove all files from to be removed
  if (toBeRemoved.size() > 0) {
    for (const auto filename : toBeRemoved) {
      std::filesystem::remove(std::string(TMP_DIR) + filename);
    }
  }
}

//  ____    _    _____ __  __  ___  _   _   _____ ___
// |  _ \  / \  | ____|  \/  |/ _ \| \ | | |_   _/ _ \/
// | | | |/ _ \ |  _| | |\/| | | | |  \| |   | || | | |
// | |_| / ___ \| |___| |  | | |_| | |\  |   | || |_| |
// |____/_/   \_\_____|_|  |_|\___/|_| \_|   |_| \___/
//
//  ____    _    _____ __  __  ___  _   _
// |  _ \  / \  | ____|  \/  |/ _ \| \ | |
// | | | |/ _ \ |  _| | |\/| | | | |  \| |
// | |_| / ___ \| |___| |  | | |_| | |\  |
// |____/_/   \_\_____|_|  |_|\___/|_| \_|
//

// We have to differentiate between Localmachine and !Localmachine as
// Localmachine will connect to all !Localmachine but !Localmachine will only
// connect to one Localmachine

static void daemonMemoryAllocationCallback(uv_handle_t *handle,
                                           size_t suggested_size,
                                           uv_buf_t *buf) {
  buf->base = (char *)malloc(suggested_size);
  buf->len = suggested_size;
}

void daemonToDaemonWriteCallback(uv_write_t *req, int status) {
  if (status < 0) {
    perror("error in daemonToDaemonWriteCallback");
    return;
  } else {
    std::cout << "written to daemon correctly";
  }
}

// This function sends read data to all nvim clients, if it is localMachine it
// also sends it to every connected daemon
void daemonToDaemonReadCallback(uv_stream_t *stream, ssize_t nread,

                                const uv_buf_t *buf) {

  // check if nread is less than 0 if it is then there is an error
  if (nread < 0) {
    // stop stream on error
    uv_read_stop(stream);
  }

  // if machine is localmachine send repeat information back to all connected
  // daemons
  if (isLocalMachine == true) {
    iterateOverStreams(connectedDaemonHandles, buf, nread);
  }
  // send to all connected neovim instances

  iterateOverStreams(connectedNvimClientsPipes, buf, nread);
}

/*     __                     __                     __    _*/
/*    / /   ____  _________ _/ /___ ___  ____ ______/ /_  (_)___  ___*/
/*   / /   / __ \/ ___/ __ `/ / __ `__ \/ __ `/ ___/ __ \/ / __ \/ _ \*/
/*  / /___/ /_/ / /__/ /_/ / / / / / / / /_/ / /__/ / / / / / / /  __/*/
/* /_____/\____/\___/\__,_/_/_/ /_/ /_/\__,_/\___/_/ /_/_/_/ /_/\___/*/

// Connection callback for daemon to daemon communication
void remoteDameonToLocalDaemonConnectionCallback(uv_connect_t *req,
                                                 int status) {
  spdlog::info("entered remoteDaemonToLocalDaemonConnectionCallback");
  if (status != 0) {
    perror("Error in remoteDaemonToLocalDaemonConnectionCallback: ");
    spdlog::error("Error in remoteDaemonToLocalDaemonConnectionCallback");
    return;
  }

  spdlog::info("req->handle type is: ", typeid(req->handle).name());

  uv_stream_t *localHandle = new uv_stream_t;
  GLOBAL_LOCAL_HANDLE = localHandle;
  localHandle = req->handle;
  uv_read_start(localHandle, daemonMemoryAllocationCallback,
                daemonToDaemonReadCallback);
}

//
//
//

void daemonToDaemonConnectionCallback(uv_stream_t *server, int status) {
  spdlog::info("Entering daemonToDaemonConnectionCallback");
  if (status < 0) {
    perror("Error in Daemon to Daemon connection callback");
    return;
  }
  uv_tcp_t *clientHandle = new uv_tcp_t;
  uv_tcp_init(loop, clientHandle);
  if (uv_accept(server, (uv_stream_t *)clientHandle) == 0) {
    spdlog::info("Accepted new client connection");

    if (uv_read_start((uv_stream_t *)clientHandle,
                      daemonMemoryAllocationCallback,
                      daemonToDaemonReadCallback) != 0) {
      perror("could not start reading from client handle");
    }
    connectedDaemonHandles.push_back((uv_stream_t *)clientHandle);
  }
  spdlog::info("Leaving daemonToDaemonConnectionCallback");
}

// listening  function
void setupListeningSocket(int port) {
  spdlog::info("entered setupListeningSocket");
  struct sockaddr_in listeningSocketAdresse = {};
  listeningSocketAdresse.sin_addr.s_addr = INADDR_ANY;
  listeningSocketAdresse.sin_port = htons(port);
  listeningSocketAdresse.sin_family = AF_INET;
  uv_tcp_t *listeningSocket = new uv_tcp_t;
  uv_tcp_init(loop, listeningSocket);
  uv_tcp_bind(listeningSocket,
              // Set flag to 2 for SO_REUSEADDR
              (const struct sockaddr *)&listeningSocketAdresse, 2);

  if (uv_listen((uv_stream_t *)listeningSocket, 128,
                daemonToDaemonConnectionCallback) != 0) {
    perror("Error in uv_listen: ");
  }
  spdlog::info("leaving setupListeningSocket");
}

//  __  __    _    ___ _   _
// |  \/  |  / \  |_ _| \ | |
// | |\/| | / _ \  | ||  \| |
// | |  | |/ ___ \ | || |\  |
// |_|  |_/_/   \_\___|_| \_|
//
int main(int argc, char *argv[]) {

  /*   __________  _   __________________*/
  /*  / ____/ __ \/ | / / ____/  _/ ____/*/
  /* / /   / / / /  |/ / /_   / // / __  */
  /*/ /___/ /_/ / /|  / __/ _/ // /_/ /  */
  /*\____/\____/_/ |_/_/   /___/\____/   */
  /*                                     */
  std::string home = getenv("HOME");
  std::string xdg_config_home = home + "/.config/nvimClipboardSync/config.toml";
  std::vector<std::string> configDirectories = {xdg_config_home};

  std::string configPath;
  int configError;

  getConfigDir(configDirectories, configPath, configError);

  auto config = toml::parse_file(configPath);

  std::optional<std::string> optCopyCmd =
      config["copyCmd"].value<std::string>();

  if (!optCopyCmd) {
    std::cout << "Error in configuration file, missing clipboard command to "
                 "use for copying to system clipboard";
  }

  std::string copyCmd = *optCopyCmd;

  /*    ___    ____  __________  ___    ____  _____ ______*/
  /*   /   |  / __ \/ ____/ __ \/   |  / __ \/ ___// ____/*/
  /*  / /| | / /_/ / / __/ /_/ / /| | / /_/ /\__ \/ __/   */
  /* / ___ |/ _, _/ /_/ / ____/ ___ |/ _, _/___/ / /___   */
  /*/_/  |_/_/ |_|\____/_/   /_/  |_/_/ |_|/____/_____/   */
  /*                                                      */

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
  uv_loop_init(loop);

  createTmpDir();

  if (isLocalMachine) {
    spdlog::info("Entered isLocalMachine");

    // set up event listener that listens to new sockets in the TMP dir (
    // sockets are created by the nvim companion plugin and are named using
    // uuid) handler that adds new sockets to a datastructure and starts reading
    // them
    uv_fs_event_t tmpDirectoryListener;
    spdlog::info("trying to set up init filesystem event listener");
    uv_fs_event_init(loop, &tmpDirectoryListener);
    spdlog::info("succesfully initialized filesystem event listener");
    spdlog::info("trying to start filesystem eventlistener");
    uv_fs_event_start(&tmpDirectoryListener, tmpDirectoryUpdateEventCallback,
                      TMP_DIR, 0);
    spdlog::info("succesfully started filesystem event listener");

    if (!optCopyCmd) {
      std::cerr
          << "Error: 'copyCmd' key missing or not a string in config file!"
          << std::endl;
      return 1;
    }
    // set up event listener that listens for the removal of sockets in the TMP

    // dir handler that removes the removed socket from the data structure

    // set up event listener that checks if nvim sockets are still used
    // if socket is no longer associated with a process remove it

    // set up event listener that listens for new tcp connections

    spdlog::info("trying to setup listening socket");
    setupListeningSocket(port);
    spdlog::info("succesfully setup listening socket");

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
    spdlog::info("trying to run uv_run()");
    uv_run(loop, UV_RUN_DEFAULT);
    spdlog::info("succesfully ran uv_run()");
  }
  // main differnece is that !isLocalMachine does not forward interrupt signals
  // sent to it to other tcp connections and it does not iterate nvim messages
  // to multiple tcp connections like isLocalMachine does
  if (!isLocalMachine) {
    spdlog::info("entered remote machine");

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

    // attempt to connect to local daemon

    struct sockaddr_in addr;
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;

    uv_connect_t *req = new uv_connect_t;
    uv_tcp_t *localConnectHandle = new uv_tcp_t;
    uv_tcp_init(loop, localConnectHandle);

    uv_tcp_connect(req, localConnectHandle, (const struct sockaddr *)&addr,
                   remoteDameonToLocalDaemonConnectionCallback);

    // start uv loop
    spdlog::info("trying to run uv_run()");
    uv_run(loop, UV_RUN_DEFAULT);
    spdlog::info("succesfully ran uv_run()");
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
