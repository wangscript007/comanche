#ifndef __DAWN_PROGRAM_OPTIONS_H__
#define __DAWN_PROGRAM_OPTIONS_H__

struct Program_options
{
  std::string data_dir;
  std::string fabric_provider; /*< e.g. verbs, socket */
  std::string device;
  std::string backend;
  std::string pci_addr;
  unsigned debug_level;
  unsigned core;
  unsigned port;
  bool devdax;
};

#endif // __DAWN_PROGRAM_OPTIONS_H__
