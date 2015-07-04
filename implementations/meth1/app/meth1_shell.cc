#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

#include "exception.hh"
#include "util.hh"

#include "implementation.hh"

#include "cluster.hh"

using namespace std;
using namespace meth1;

using size_type = Implementation::size_type;

void run( int argc, char * argv[] );
void shell( Cluster & c, size_type records, size_type block_size,
            string out_dir );

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc <= 5 ) {
    throw runtime_error(
      "Usage: " + string( argv[0] ) +
      " [size] [read ahead] [block size] [out folder] [nodes...]" );
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

void run( int argc, char * argv[] )
{
  // check started OK
  check_usage( argc, argv );

  // get arguments
  size_type records = stoul( argv[1] );
  size_type read_ahead = stoul( argv[2] );
  size_type block_size = stoul( argv[3] );
  string out_dir{argv[4]};
  char ** addresses = argv + 5;

  // setup out directory
  mkdir( out_dir.c_str(), 0755 );

  // setup cluster
  auto addrs = vector<Address>( addresses, argv + argc );
  Cluster client{addrs, read_ahead};

  // run shell
  while ( true ) {
    shell( client, records, block_size, out_dir );
    if ( cin.eof() ) {
      break;
    }
  }
}

File query_file( string out_dir, unsigned int query, string type )
{
  string out = out_dir + "/q-" + to_string( query ) + "-" + type;
  return {out, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR};
}

void shell( Cluster & c, size_type records, size_type block_size,
            string out_dir )
{
  static unsigned int query = 0;

  string line, str;
  cout << "> ";
  getline( cin, line );
  istringstream iss{line};

  iss >> str;
  if ( str == "start" ) {
    // 'start\n'         -- prepare the system
    auto start = chrono::high_resolution_clock::now();

    c.Initialize();

    auto end = chrono::high_resolution_clock::now();
    auto dur = chrono::duration_cast<chrono::seconds>( end - start ).count();
    cout << dur << endl;

  } else if ( str == "all*" ) {
    // 'all\n'           -- get all records (but don't write!)
    auto start = chrono::high_resolution_clock::now();

    for ( size_type i = 0; i < records; i += block_size ) {
      vector<Record> recs = c.Read( i, block_size );
    }

    auto end = chrono::high_resolution_clock::now();
    auto dur = chrono::duration_cast<chrono::seconds>( end - start ).count();
    cout << dur << endl;

  } else if ( str == "all" ) {
    // 'all\n'           -- get all records
    auto start = chrono::high_resolution_clock::now();

    File out = query_file( out_dir, query, "all" );

    for ( size_type i = 0; i < records; i += block_size ) {
      vector<Record> recs = c.Read( i, block_size );

      auto t1 = chrono::high_resolution_clock::now();
      for ( const auto & r : recs ) {
        out.write( r.str( Record::NO_LOC ) );
      }
      auto t2  = chrono::high_resolution_clock::now();
      auto dur = chrono::duration_cast<chrono::milliseconds>( t2 - t1 ).count();
      cout << "* File write took: " << dur << "ms" << endl;
    }

    auto end = chrono::high_resolution_clock::now();
    auto dur = chrono::duration_cast<chrono::seconds>( end - start ).count();
    cout << dur << endl;

  } else if ( str == "record" ) {
    // 'record [int]\n'  -- get record n
    cout << "not implemented!" << endl;

  } else if ( str == "first" ) {
    // 'first\n'         -- get first record
    auto start = chrono::high_resolution_clock::now();

    File out = query_file( out_dir, query, "first" );
    Record r = c.Read( 0, 1 )[0];
    out.write( r.str( Record::NO_LOC ) );

    auto end = chrono::high_resolution_clock::now();
    auto dur = chrono::duration_cast<chrono::seconds>( end - start ).count();
    cout << dur << endl;

  } else {
    return;
  }

  query++;
}
