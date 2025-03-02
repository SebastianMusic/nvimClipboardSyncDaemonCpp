// TODO: Maybe not have a config file on remote machine since we dont need to
// sync clipboards there or atleast have an option to turn clipboard syncing off
//
// TODO: need to improve timer precission more detailed unix time stamps
//
// TODO: refactor function names and make it more logical
//
// TODO: create a function to create a empty config file if it does not already
// exist
//

#include "includeAllRapidJson.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include <algorithm>
#include <argparse/argparse.hpp>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <netinet/in.h>
#include <pthread.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <toml.hpp>
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
  spdlog::info("Entered getConfigDir");

  for (const std::string &path : configDirectories) {
    if (std::filesystem::exists(path)) {
      configDir = path;
      error = 0;
      return;
    }
  }
  error = -1;
  spdlog::info("Leaving getConfigDir");
}
/*   __  ______________    ____________  __*/
/*  / / / /_  __/  _/ /   /  _/_  __/\ \/ /*/
/* / / / / / /  / // /    / /  / /    \  / */
/*/ /_/ / / / _/ // /____/ /  / /     / /  */
/*\____/ /_/ /___/_____/___/ /_/     /_/   */
/*                                         */
std::vector<std::string> getFilenamesInDirectory(const std::string &directory) {
  spdlog::info("Entered getFilenamesInDirectory");
  std::vector<std::string> filenames;

  for (const auto &entry : std::filesystem::directory_iterator(directory)) {

    filenames.push_back(entry.path().filename().string());
  }
  return filenames;
  spdlog::info("Leaving getFilenamesInDirectory");
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
std::vector<uv_stream_t *> connectedNvimHandles;
std::string copyCmd;
long long TIMESTAMP;

// hopefully unique enough tmp directory
#define TMP_DIR "/tmp/com.sebastianmusic.nvimclipboardsync/"
void createTmpDir() {
  spdlog::info("Entered createTmpDir");
  struct stat st;
  if (stat(TMP_DIR, &st) == 0) {
    if (!S_ISDIR(st.st_mode)) {
      std::cerr << "Error: " TMP_DIR
                   " exists but is not a directory. Please remove it.\n";
      exit(1);
    }
    spdlog::info("Temporary directory already exists: {}", TMP_DIR);
  } else {
    if (mkdir(TMP_DIR, 0700) != 0) {
      perror("Failed to create tmp directory");
      exit(1);
    }
    spdlog::info("Created Temporary directory: {}", TMP_DIR);
  }
  spdlog::info("Leaving createTmpDir");
}

void iterateOverStreams(std::vector<uv_stream_t *> vectorToIterateOver,
                        char *buf, ssize_t nread, std::string vectorName) {
  spdlog::info("Entered iterateOverStreams");
  if (buf != NULL) {
    spdlog::info("buffer is {}", std::string(buf));
  } else {
    spdlog::info("buffer is empty");
  }
  spdlog::info("working with vector: {}", vectorName);
  spdlog::info("nread in iterateOverStreams is {}", nread);
  spdlog::info("amount of vectors to iterate over are {}",
               vectorToIterateOver.size());

  for (uv_stream_t *handle : vectorToIterateOver) {

    uv_write_t *writeRequest = new uv_write_t();
    uv_buf_t bufs = uv_buf_init(buf, nread);
    spdlog::info("bufs in iterateOverStreams is: {} nread in bufs is: {}",
                 std::string(bufs.base, nread), nread);

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
  spdlog::info("Leaving iterateOverStreams");
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
  spdlog::info("entered nvimMemoryAllocationCallback");
  buf->base = (char *)malloc(suggested_size);
  buf->len = suggested_size;
  spdlog::info("Leaving nvimMemoryAllocationCallback");
}

void nvimCloseCallback(uv_handle_t *handle) {
  spdlog::info("Entered nvimCloseCallback");
  spdlog::info("Freeing nvimHandle");
  free(handle);
  spdlog::info("Removing handle from connectedNvimHandles");
  connectedNvimHandles.erase(std::remove(connectedNvimHandles.begin(),
                                         connectedNvimHandles.end(),
                                         (uv_stream_t *)handle),
                             connectedNvimHandles.end());
  spdlog::info("Leaving nvimCloseCallback");
}

void readFromNvim(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
  spdlog::info("entered readFromNvim");
  // check if nread is less than 0 if it is then there is an error
  if (nread < 0) {
    // stop stream on error
    uv_read_stop(stream);
    spdlog::info("nread is less than 0 error");
    spdlog::info("attemtping uv_close then returning from function");
    uv_close((uv_handle_t *)stream, nvimCloseCallback);
    return;
  }

  char *buffer = new char[nread + 1];
  memcpy(buffer, buf->base, nread);
  buffer[nread] = '\0';
  spdlog::info("uv_buf_t in readFromNvim is {}",
               std::string(buf->base, buf->len));
  spdlog::info("nread in readFromNvim is {}", nread);

  spdlog::info("char buffer in readFromNvim {}", buffer);
  iterateOverStreams(connectedDaemonHandles, buffer, nread,
                     "connectedDaemonHandles");
  delete[] buffer;
  spdlog::info("freeing buf->base in readFromNvim");
  free(buf->base);
  spdlog::info("Leaving readFromNvim");
}

void nvimConnectCallback(uv_connect_t *req, int status) {
  spdlog::info("Entered nvimConnectCallback");
  if (status < 0) {
    spdlog::error("error occured in connection callback could not connect");
    return;
  }
  uv_read_start(req->handle, nvimMemoryAllocationCallback, readFromNvim);
  spdlog::info("Leaving nvimConnectCallback");
}

typedef std::map<std::string, uv_stream_t *> PipenameToHandle;

std::vector<PipenameToHandle> connectedNvimClientsPipesMap;
std::vector<uv_stream_t *> connectedNvimClientsPipesDeprecated;
void daemonToNvimConnectionCallback(uv_stream_t *server, int status) {
  spdlog::info("entered daemonToNvimConnectionCallback");
  if (status < 0) {
    spdlog::error(
        "error occured in daemonToNvimConnectionCallback could not connect");
    return;
  }
  uv_pipe_t *nvimClient = new uv_pipe_t;
  uv_pipe_init(loop, nvimClient, 0);

  if (uv_accept(server, (uv_stream_t *)nvimClient) == 0) {
    spdlog::info("Accepted new client connection");

    if (uv_read_start((uv_stream_t *)nvimClient, nvimMemoryAllocationCallback,
                      readFromNvim) != 0) {
      perror("error reading from neovim client");
      spdlog::error("could not read from neovim client RETURNING");
      return;
    }

    connectedNvimHandles.push_back((uv_stream_t *)nvimClient);
  }

  spdlog::info("Leaving daemonToNvimConnectionCallback");
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
  spdlog::info("Entered daemonMemoryAllocatioCallback");
  spdlog::info("allocating suggested size {}", suggested_size);
  buf->base = (char *)malloc(suggested_size);
  buf->len = suggested_size;
  spdlog::info("Leaving daemonMemoryAllocatioCallback");
}

void daemonToDaemonWriteCallback(uv_write_t *req, int status) {
  spdlog::info("Entered daemonToDaemonWriteCallback");
  if (status < 0) {
    perror("error in daemonToDaemonWriteCallback");
    return;
  } else {
    spdlog::info("written memory to daemon correctly");
  }
  spdlog::info("Leaving daemonToDaemonWriteCallback");
}

void validateBuffer(char *buf, ssize_t nread) {
  spdlog::info("Entered validateBuffer");
  char buffer[nread + 1];
  int i = 0;
  while (i < nread) {
    buffer[i] = buf[i];

    if (buf[i] == '}') {
      buffer[i + 1] = '\0';
      // Try to parse it to see if its a valid json object
      rapidjson::Document doc;
      doc.Parse(buffer);

      if (!doc.HasParseError()) {
        memcpy(buf, buffer, i + 1);
        i++;
        buf[i] = '\0';
        spdlog::info("Validated buffer in validateBuffer is: {}",
                     std::string(buf, i));
        spdlog::info("Leaving validateBuffer json object succseffully found");
        return;
      }
    }
    i++;
  }
  buf[0] = '\0';
  spdlog::error("Leaving validateBuffer no json object found");
}

void copyToSystemClipboard(std::string copyCmd, char *buf, ssize_t nread) {
  spdlog::info("entered copyToSystemClipboard");
  spdlog::info("bytes read: {}", nread);
  spdlog::info("json is below");
  spdlog::info("json is {}", buf);
  spdlog::info("json is above");
  spdlog::info("jsonStr is {}", buf);
  std::string jsonString = std::string(buf);

  rapidjson::Document doc;

  doc.Parse(buf);
  if (doc.HasParseError()) {
    spdlog::error("JSON parse error: {}",
                  rapidjson::GetParseError_En(doc.GetParseError()));
    validateBuffer(buf, nread);
    doc.Parse(buf);
  }
  spdlog::info("attempting assertions in copyToSystemCLipboard");
  spdlog::info("attemting to assert odc.IsObject");
  doc.IsObject();
  spdlog::info("succsefully asserted odc.IsObject");
  // NOTE: Changing so that timestamp is a string and i convert it to an int on
  // the server
  spdlog::info("attempting to assert HasMember(TIMESTAMP)");

  assert(doc.HasMember("TIMESTAMP"));
  spdlog::info("succsefully asserted HasMember(TIMESTAMP)");
  spdlog::info("attempting to assert (TIMESTAMP).IsString()");
  assert(doc["TIMESTAMP"].IsString());
  spdlog::info("Sucseffully asserted (TIMESTAMP).IsString()");
  spdlog::info("attempting to assert HasMember(REGISTER)");
  assert(doc.HasMember("REGISTER"));
  spdlog::info("succsefully asserted HasMember(REGISTER)");
  spdlog::info("attempting to assert (Register).IsString()");
  assert(doc["REGISTER"].IsString());
  spdlog::info("succsesfully asserted (Register).IsString()");
  spdlog::info("succsessfully asserted everything in copyToSystemClipboard");

  // if the new timestamp is newer than the current one then copy it into the
  // clipboard if not then discard it

  long long tmpTIMESTAMP = std::stoll(doc["TIMESTAMP"].GetString());
  if (tmpTIMESTAMP > TIMESTAMP) {
    spdlog::info("timestamp is newer, updating clipboard");
    std::string REGISTER = doc["REGISTER"].GetString();
    // open the clipboard command as an external proccess in writing mode
    // safer than shell execution
    spdlog::info("copy command to try is {}", copyCmd);
    FILE *copyProcess = popen(copyCmd.c_str(), "w");
    if (!copyProcess) {
      spdlog::error("Failed to open clipboard process");
      return;
    }
    fwrite(REGISTER.c_str(), sizeof(char), REGISTER.length(), copyProcess);
    fflush(copyProcess); // Ensure data is written
    pclose(copyProcess); // Close process safely

    TIMESTAMP = tmpTIMESTAMP;
  } else {
    spdlog::warn("timestamp is NOT newer, NOT updating clipboard");
  }

  spdlog::info("Leaving copyToSystemClipboard");
}

void daemonToDaemonCloseCallback(uv_handle_t *handle) {
  spdlog::info("Entered daemonToDaemonCloseCallback");
  spdlog::info("attemtping to free might be issue with malloc and new memory "
               "allocation mismatch");
  free(handle);
  spdlog::info("attemtping to remove dameon from datastructure");
  connectedDaemonHandles.erase(std::remove(connectedDaemonHandles.begin(),
                                           connectedDaemonHandles.end(),
                                           (uv_stream_t *)handle),
                               connectedDaemonHandles.end());
  spdlog::info("Leaving daemonToDaemonCloseCallback");
}

void daemonToDaemonReadCallback(uv_stream_t *stream, ssize_t nread,
                                const uv_buf_t *buf) {
  spdlog::info("entering daemonToDaemonReadCallback");

  // check if nread is less than 0 if it is then there is an error
  if (nread < 0) {
    // stop stream on error
    uv_read_stop(stream);
    daemonToDaemonCloseCallback((uv_handle_t *)stream);
    return;
  }
  if (!buf->base) {
    spdlog::error("Recevied nul lbuffer in daemonToDaemonReadcallback");
    return;
  }

  spdlog::info("nread in daemonToDaemonReadCallback is {}", nread);
  spdlog::info("allocating buffer size: {}", nread + 1);
  char *buffer = new char[nread + 1];
  memcpy(buffer, buf->base, nread);
  buffer[nread] = '\0';
  spdlog::info("uv_buf_t in daemonToDaemonReadCallback is {}",
               std::string(buf->base, buf->len));
  spdlog::info("buffer in daemonToDaemonReadCallback is {}", buffer);

  // if machine is localmachine send repeat information back to all connected
  // daemons
  spdlog::info("isLocalMachine is: {}", isLocalMachine);
  if (isLocalMachine == true) {
    // NOTE: disabled this currently to stop errors but it also means that its
    // only possible with a maximum of one remote machine instead of multiple
    // iterateOverStreams(connectedDaemonHandles, buffer, nread);
    copyToSystemClipboard(copyCmd, buffer, nread);
  }
  // send to all connected neovim instances

  iterateOverStreams(connectedNvimHandles, buffer, nread,
                     "connectedNvimHandles");
  delete[] buffer;
  spdlog::info("attempting to free buf->base in daemonTodaemonReadCallback to "
               "see if resolves issue");
  free(buf->base);
  spdlog::info("Leaving daemonToDaemonReadCallback");
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

  int vectorSize = connectedDaemonHandles.size();
  spdlog::info("vector size on remoteDaemon is: {}", vectorSize);
  if (vectorSize > 1) {
    spdlog::error("TRIED TO CONNECT TO A SECOND LOCAL DAEMON, UNDEFINED "
                  "BEHAVIOUR EXITING");
    exit(-1);
  }
  connectedDaemonHandles.push_back((uv_stream_t *)localHandle);
  spdlog::info("Leaving remoteDaemonToLocalDaemonConnectionCallback");
}

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

void setupListeningPipe() {
  spdlog::info("entered setupListeningPipe");
  std::filesystem::path path = std::string(TMP_DIR) + "listeningPipe";
  if (std::filesystem::exists(path)) {
    spdlog::info("listeningPipe already exists, removing it");
    std::filesystem::remove(path);
  }

  uv_pipe_t *listeningPipe = new uv_pipe_t;
  uv_pipe_init(loop, listeningPipe, 0);
  uv_pipe_bind(listeningPipe, path.c_str());
  if (uv_listen((uv_stream_t *)listeningPipe, 128,
                daemonToNvimConnectionCallback) != 0) {
    perror("Error in uv_listen: ");
  }
  spdlog::info("Leaving setupListeningPipe");
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
    spdlog::info("Error in configuration file, missing clipboard command to "
                 "use for copying to system clipboard");
  }

  copyCmd = *optCopyCmd;

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
    spdlog::error("Error: {}", err.what());
    return 1;
  }

  int port = program.get<int>("-p");
  isLocalMachine = program["--isLocalMachine"] == true;
  spdlog::info("port is {}", port);
  if (isLocalMachine == true) {
    spdlog::info("Running on local machine\n");
  } else {
    spdlog::info("Running on remote machine\n");
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

    if (!optCopyCmd) {
      std::cerr
          << "Error: 'copyCmd' key missing or not a string in config file!"
          << std::endl;
      return 1;
    }

    spdlog::info("trying to setup listening socket");
    setupListeningSocket(port);
    spdlog::info("succesfully setup listening socket");

    spdlog::info("trying to setup listening pipe");
    setupListeningPipe();
    spdlog::info("succesfully setup listening pipe");

    // start uv loop
    spdlog::info("starting uv_run()");
    uv_run(loop, UV_RUN_DEFAULT);
    spdlog::info("succesfully ran uv_run()");
  }
  // main differnece is that !isLocalMachine does not forward interrupt
  // signals sent to it to other tcp connections and it does not iterate nvim
  // messages to multiple tcp connections like isLocalMachine does
  if (!isLocalMachine) {
    spdlog::info("entered remote machine");

    spdlog::info("trying to setup listening pipe");
    setupListeningPipe();
    spdlog::info("succesfully setup listening pipe");
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
    spdlog::info("Starting run uv_run()");
    uv_run(loop, UV_RUN_DEFAULT);
    spdlog::info("succesfully ran uv_run()");
  }

  return 0;
}
