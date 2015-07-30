#include <sys/stat.h>
#include <sys/types.h>

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

#include "exception.hh"

#include "implementation.hh"

#include "cluster.hh"

using namespace std;
using namespace meth1;

File query_file( string out_dir, unsigned int query, string type )
{
  string out = out_dir + "/q-" + to_string( query ) + "-" + type;
  return {out, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR};
}

void shell( Cluster & c, string out_dir )
{
  static unsigned int query = 0;

  string line, str;
  cout << "> ";
  getline( cin, line );
  istringstream iss{line};

  iss >> str;
  if ( str == "first" ) {
    auto start = chrono::high_resolution_clock::now();
    Record r = c.ReadFirst();
    cout << r << endl;
    auto end = chrono::high_resolution_clock::now();
    auto dur = chrono::duration_cast<chrono::seconds>( end - start ).count();
    cout << dur << endl;

  } else if ( str == "read" ) {
    auto start = chrono::high_resolution_clock::now();

    c.ReadAll();

    auto end = chrono::high_resolution_clock::now();
    auto dur = chrono::duration_cast<chrono::seconds>( end - start ).count();
    cout << dur << endl;

  } else if ( str == "write" ) {
    auto start = chrono::high_resolution_clock::now();
    File out = query_file( out_dir, query, "all" );

    c.WriteAll( move( out ) );

    auto end = chrono::high_resolution_clock::now();
    auto dur = chrono::duration_cast<chrono::seconds>( end - start ).count();
    cout << dur << endl;

  } else if ( str == "size" ) {
    auto start = chrono::high_resolution_clock::now();

    cout << c.Size() << endl;

    auto end = chrono::high_resolution_clock::now();
    auto dur = chrono::duration_cast<chrono::seconds>( end - start ).count();
    cout << dur << endl;

  } else {
    return;
  }

  query++;
}

void run( int argc, char * argv[] )
{
  // get arguments
  uint64_t read_ahead = stoul( argv[1] );
  string out_dir{argv[2]};
  char ** addresses = argv + 3;

  // setup out directory
  mkdir( out_dir.c_str(), 0755 );

  // setup cluster
  auto addrs = vector<Address>( addresses, argv + argc );
  Cluster client{addrs, read_ahead};

  // run shell
  while ( true ) {
    shell( client, out_dir );
    if ( cin.eof() ) {
      break;
    }
  }
}

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc < 4 ) {
    throw runtime_error(
      "Usage: " + string( argv[0] ) +
      " [read ahead] [out folder] [nodes...]" );
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

