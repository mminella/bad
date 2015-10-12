#include <iostream>
#include <random>
#include <string>

#include "file.hh"
#include "circular_io_var.hh"
#include "exception.hh"
#include "util.hh"

using namespace std;

void run( char * fin )
{
  File file( fin, O_RDONLY );
  CircularIOVar rio( file, 100 );
  rio.start_read( file.size() );

  random_device rd;
  mt19937_64 gen(rd());
  uniform_int_distribution<> dist(0,255);

  size_t sizes[255];
  zero( sizes );
  
  while ( true ) {
    auto rec = rio.next_record();
    if ( rec.first == nullptr ) {
      break;
    }
    sizes[dist(gen)]++;
  }

  for ( size_t i = 0; i < 255; i++ ) {
    cout << "Size " << i+1 << ": " << sizes[i] << endl;
  }
}

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc != 2 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) + " [file]" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    check_usage( argc, argv );
    run( argv[1] );
  } catch ( const exception & e ) {
    print_exception( e );
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
