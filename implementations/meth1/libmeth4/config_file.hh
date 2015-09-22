#ifndef METH4_CONFIG_FILE_HH
#define METH4_CONFIG_FILE_HH

#include <string>
#include <vector>

#include "address.hh"

class ConfigFile
{
private:
  uint16_t myID_;
  std::vector<Address> cluster_;

public:
  ConfigFile( std::string file, std::string hostname );

  /* My ID (and position in vector) in the cluster. */
  uint16_t myID( void ) const noexcept;

  /* List of addresses (including SELF) in the cluster. */
  std::vector<Address> cluster( void ) const noexcept;
};

#endif /* METH4_CONFIG_FILE_HH */
