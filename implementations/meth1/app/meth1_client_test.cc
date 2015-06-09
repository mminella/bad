#include <fcntl.h>

#include <algorithm>
#include <thread>
#include <vector>

#include "exception.hh"
#include "poller.hh"
#include "util.hh"

#include "record.hh"

#include "client.hh"

using namespace std;
using namespace meth1;

int run( int argc, char * argv[] );

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc != 3 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) +
                         " [node] [out file]" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    run( argc, argv );
  } catch ( const exception & e ) {
    print_exception( e );
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int run( int argc, char * argv[] )
{
  sanity_check_env( argc );
  check_usage( argc, argv );

  File out( argv[2], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR );

  Client client{argv[1]};
  client.Initialize();

  // // read directly (no poller)
  // for ( size_t i = 0; i < 1000; i++ ) {
  //   client.sendRead( i, 1 );
  //   auto recs = client.recvRead();
  //   out.write( recs[0].str( Record::NO_LOC ) );
  // }

  // run RPC
  Poller poller;
  poller.add_action( client.RPCRunner() );
  thread fileRPC( [&poller]() {
    while ( true ) {
      poller.poll( -1 );
    }
  } );
  fileRPC.detach();

  // read individually
  // for ( size_t i = 0; i < 1000; i++ ) {
  //   auto recs = client.Read( i, 1 );
  //   out.write( recs[0].str( Record::NO_LOC ) );
  // }

  // read batch
  auto recs = client.Read( 0, 1000 );
  cout << "Recs: " << recs.size() << endl;

  for ( auto & r : recs ) {
    out.write( r.str( Record::NO_LOC ) );
  }

  return EXIT_SUCCESS;
}
