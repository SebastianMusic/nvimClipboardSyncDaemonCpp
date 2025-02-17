
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdio.h>

int main() {
  std::string home = getenv("HOME");
  std::cout << std::filesystem::exists(
      home + "/.config/nvimClipboardSync/config.toml");

  return 1;
}
