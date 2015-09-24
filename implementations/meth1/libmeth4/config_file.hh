#ifndef METH4_CONFIG_FILE_HH
#define METH4_CONFIG_FILE_HH

#include <string>
#include <vector>

#include "address.hh"

namespace ConfigFile {
  /* List of addresses (including SELF) in the cluster. */
  std::vector<Address> parse( std::string file );
};

#endif /* METH4_CONFIG_FILE_HH */
