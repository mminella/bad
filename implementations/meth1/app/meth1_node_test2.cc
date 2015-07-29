/**
 * Do a local machine sort using the method1::Node backend.
 *
 * Variation that overlaps read+write of sorted records.
 */
#include <chrono>
#include <iostream>
#include <system_error>
#include <thread>

#include "channel.hh"
#include "record.hh"

#include "node.hh"

using namespace std;
using namespace meth1;

using clk = chrono::high_resolution_clock;

static constexpr size_t MAX_MEM = 2097152; // 200MB

using Recs = vector<Record>;

clk::duration ttr, ttw;

// reader co-ordinator -- seperate thread to allow R+W overlap.
void reader( Node * node, size_t block_size,
             Channel<size_t> req, Channel<Recs> resp )
{
  while ( true ) {
    size_t pos;
    try {
      pos = req.recv();
    } catch ( const std::exception & e ) {
        return;
    }
    auto tr = clk::now();
    auto recs = node->Read( pos, block_size );
    ttr += clk::now() - tr;
    resp.send( move( recs ) );
  }
}

int run( char * fin, char * fout, double block )
{
  BufferedIO_O<File> out( {fout, O_WRONLY | O_CREAT | O_TRUNC,
                                 S_IRUSR | S_IWUSR} );
  auto t0 = clk::now();

  // start node
  Node node{fin, "0", MAX_MEM};
  node.Initialize();
  auto t1 = clk::now();

  // size
  auto siz = node.Size();
  auto t2 = clk::now();

  // read + write
  if ( siz > 0 ) {
    Channel<size_t> req( 0 );
    Channel<Recs> resp( 0 );
    size_t block_size = block * siz;
    thread rthread( reader, &node, block_size, req, resp );

    req.send( 0 );
    for ( size_t i = block_size; i < siz; i += block_size ) {
      auto recs = resp.recv();
      req.send( i ); // send now to overlap with writing.

      auto tw = clk::now();
      for ( auto & r : recs ) {
        r.write( out );
      }
      out.flush( true );
      out.io().fsync();
      ttw += clk::now() - tw;
    }

    // last block
    auto recs = resp.recv();
    auto tw = clk::now();
    for ( auto & r : recs ) {
      r.write( out );
    }
    out.flush( true );
    out.io().fsync();
    ttw += clk::now() - tw;
    
    // done
    req.close();
    rthread.join();
  }

  auto t3 = clk::now();

  // stats
  auto tinit = chrono::duration_cast<chrono::milliseconds>( t1 - t0 ).count();
  auto tsize = chrono::duration_cast<chrono::milliseconds>( t2 - t1 ).count();
  auto tread = chrono::duration_cast<chrono::milliseconds>( ttr ).count();
  auto twrit = chrono::duration_cast<chrono::milliseconds>( ttw ).count();
  auto ttota = chrono::duration_cast<chrono::milliseconds>( t3 - t0 ).count();

  cout << "-----------------------" << endl;
  cout << "Records " << siz << endl;
  cout << "-----------------------" << endl;
  cout << "Start took " << tinit << "ms" << endl;
  cout << "Size  took " << tsize << "ms" << endl;
  cout << "Read  took " << tread << "ms (read + sort)" << endl;
  cout << "Write took " << twrit << "ms" << endl;
  cout << "Total took " << ttota << "ms" << endl;
  cout << "Writ calls (buf) " << out.write_count() << endl;
  cout << "Writ calls (dir) " << out.io().write_count() << endl;

  return EXIT_SUCCESS;
}

void check_usage( const int argc, const char * const argv[] )
{
  if ( argc != 3 and argc != 4 ) {
    throw runtime_error( "Usage: " + string( argv[0] ) +
                         " [file] [out file] ([block %])" );
  }
}

int main( int argc, char * argv[] )
{
  try {
    check_usage( argc, argv );
    double block = argc == 4 ? stod( argv[3] ) : 1;
    run( argv[1], argv[2], block );
  } catch ( const exception & e ) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

