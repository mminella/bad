#ifndef RPC_HH
#define RPC_HH

#include <cstdint>

namespace meth1
{

enum RPC : int8_t {
  READ,
  SIZE,
  MAX_CHUNK,
  EXIT
};

}

#endif /* RPC_HH */
