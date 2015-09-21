#include <fcntl.h>

#include <algorithm>
#include <iostream>
#include <vector>

#include "exception.hh"
#include "util.hh"

#include "record.hh"

#include "cluster.hh"

using namespace std;
using namespace meth2;

int run( int argc, char * argv[] );

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc < 4 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) +
                         " [read ahead] [out file] [nodes...]" );
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
  check_usage( argc, argv );

  size_t readahead = stoul( argv[1] );
  string ofile{argv[2]};
  char ** addresses = argv + 3;

  File out( ofile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR );

  auto addrs = vector<Address>( addresses, argv + argc );
  Cluster c{addrs, readahead};
 
  auto start = time_now();
  auto size = c.Size();
  auto diff = time_diff<ms>(start);
  cout << "Size: " << size << endl;
  cout << "Size: " << diff << "mS" << endl;

  start = time_now();
  Record r = c.ReadFirst();
  diff = time_diff<ms>(start);
  cout << "Record: " << r << endl;
  cout << "First: " << diff << "mS" << endl;

  start = time_now();
  c.GetSplit(500);
  c.GetSplit(1000);
  c.GetSplit(5000);
  diff = time_diff<ms>(start);
  cout << "Split: " << diff << "mS" << endl;

  start = time_now();
  c.Read(5, 10);
  diff = time_diff<ms>(start);
  cout << "Range: " << diff << "mS" << endl;

  start = time_now();
  c.ReadAll();
  diff = time_diff<ms>(start);
  cout << "All: " << diff << "mS" << endl;

  //c.WriteAll( move( out ) );

  return EXIT_SUCCESS;
}

