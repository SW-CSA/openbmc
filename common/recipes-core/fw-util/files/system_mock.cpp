#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "fw-util.h"

#define PAGE_SIZE                     0x1000
#define VERIFIED_BOOT_STRUCT_BASE     0x1E720000
#define VERIFIED_BOOT_HARDWARE_ENFORCE(base) \
  *((uint8_t *)(base + 0x215))

using namespace std;

// TODO Mocks are yet to be written

int System::runcmd(const string &cmd)
{
  cout << "Running: " << cmd << endl;
  return FW_STATUS_SUCCESS;
}

bool System::vboot_hardware_enforce(void)
{
  const char *env = std::getenv("FWUTIL_HWENFORCE");
  if (env) {
    return true;
  }
  return false;
}
bool System::get_mtd_name(const char* _name, char* dev)
{
  map<string, string> mtd_map = {
    {"\"flash0\"", "/dev/mtd5"},
  };
  if (std::getenv("FWUTIL_VBOOT") ||
      std::getenv("FWUTIL_DUALFLASH")) {
    mtd_map["\"flash1\""] = "/dev/mtd12";
  }
  if (std::getenv("FWUTIL_VBOOT")) {
    mtd_map["\"romx\""] = "/dev/mtd0";
  }

  string name(_name);
  if (mtd_map.find(name) == mtd_map.end()) {
    return false;
  }
  if (dev) {
    strcpy(dev, mtd_map[name].c_str());
  }
  return true;
}

string& System::name()
{
  static string ret = "fbtp";
  const char *env = std::getenv("FWUTIL_MACHINE");
  if (env) {
    ret = env;
  }
  return ret;
}

string& System::partition_conf()
{
  static string parts = "./image_parts.json";
  return parts;
}

uint8_t System::get_fru_id(string &name)
{
  const char *env = std::getenv("FWUTIL_FRUID");
  if (!env) {
    return 1;
  }
  return atoi(env);
}

string& System::lock_file(string &name)
{
  static string f = "./fw-util.lock";
  return f;
}

void System::set_update_ongoing(uint8_t fru_id, int timeo)
{
}