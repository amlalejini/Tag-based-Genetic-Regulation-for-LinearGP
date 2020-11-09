//  This file is part of SignalGP Genetic Regulation.
//  Copyright (C) Alexander Lalejini, 2019.
//  Released under MIT license; see LICENSE

#include <iostream>
#include <string>

#include "emp/base/vector.hpp"
#include "emp/config/ArgManager.hpp"
#include "emp/config/command_line.hpp"

#include "../DirSignalWorld.h"
#include "../DirSignalConfig.h"

int main(int argc, char* argv[])
{
  std::string config_fname = "config.cfg";
  DirSigConfig config;
  auto args = emp::cl::ArgManager(argc, argv);
  config.Read(config_fname);
  if (args.ProcessConfigOptions(config, std::cout, config_fname, "config-macros.h") == false) exit(0);
  if (args.TestUnknown() == false) exit(0); // If there are leftover args, throw an error.

  // Write to screen how the experiment is configured
  std::cout << "==============================" << std::endl;
  std::cout << "|    How am I configured?    |" << std::endl;
  std::cout << "==============================" << std::endl;
  config.Write(std::cout);
  std::cout << "==============================\n" << std::endl;

  emp::Random random(config.SEED());
  DirSigWorld world(random);
  world.Setup(config);
  world.Run();
}