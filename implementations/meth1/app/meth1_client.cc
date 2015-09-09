#include <sys/stat.h>
#include <sys/types.h>

#include <cstdlib>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

#include "exception.hh"
#include "sync_print.hh"

#include "cluster.hh"

using namespace std;
using namespace meth1;

File query_file( string out_dir, unsigned int query, string type )
{
  string out = out_dir + "/q-" + to_string( query ) + "-" + type;
  return {out, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR};
}

void run_cmd( Cluster & c, string out_dir, string cmd, uint64_t read_ahead )
{
  if ( cmd == "first" ) {
    auto start = chrono::high_resolution_clock::now();
    Record r = c.ReadFirst();
    auto end = chrono::high_resolution_clock::now();
    auto dur = chrono::duration_cast<chrono::milliseconds>( end - start ).count();
    print( "\nfirst", r );
    print( "cmd-first", dur );

  } else if ( cmd == "read" ) {
    auto start = chrono::high_resolution_clock::now();
    c.ReadAll();
    auto end = chrono::high_resolution_clock::now();
    auto dur = chrono::duration_cast<chrono::milliseconds>( end - start ).count();
    print( "\ncmd-read", dur );

  } else if ( cmd == "chunk" ) {
    auto start = chrono::high_resolution_clock::now();
    c.Read( 0, read_ahead );
    auto end = chrono::high_resolution_clock::now();
    auto dur = chrono::duration_cast<chrono::milliseconds>( end - start ).count();
    print( "\ncmd-chunk", dur );

  } else if ( cmd.find_first_of( "chunk-" ) == 0 ) {
    size_t i = cmd.find_last_of( '-' );
    auto siz = atol( cmd.substr( i + 1 ).c_str() );

    auto start = chrono::high_resolution_clock::now();
    c.Read( 0, siz );
    auto end = chrono::high_resolution_clock::now();
    auto dur = chrono::duration_cast<chrono::milliseconds>( end - start ).count();
    print( "\ncmd-chunk", dur );

  } else if ( cmd == "write" ) {
    auto start = chrono::high_resolution_clock::now();
    File out = query_file( out_dir, 0, "all" );
    c.WriteAll( move( out ) );
    auto end = chrono::high_resolution_clock::now();
    auto dur = chrono::duration_cast<chrono::milliseconds>( end - start ).count();
    print( "\ncmd-write", dur );
  }
}

void run( int argc, char * argv[] )
{
  // get arguments
  uint64_t read_ahead = stoul( argv[1] );
  string out_dir{argv[2]};
  string cmd{argv[3]};
  char ** addresses = argv + 4;

  // setup out directory
  mkdir( out_dir.c_str(), 0755 );

  // setup cluster
  auto addrs = vector<Address>( addresses, argv + argc );
  Cluster client{addrs, read_ahead};

  // run cmd
  run_cmd( client, out_dir, cmd, read_ahead );
  client.Shutdown();
}

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc < 5 ) {
    throw runtime_error(
      "Usage: " + string( argv[0] ) +
      " [read ahead] [out folder] [cmd] [nodes...]" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    check_usage( argc, argv );
    run( argc, argv );
  } catch ( const exception & e ) {
    print_exception( e );
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

