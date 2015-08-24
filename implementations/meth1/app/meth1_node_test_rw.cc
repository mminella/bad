/**
 * Do a local machine sort using the method1::Node backend + write out
 * (overlapped) to disk.
 */
#include <iostream>
#include <system_error>
#include <thread>

#include "channel.hh"
#include "timestamp.hh"
#include "record.hh"
#include "node.hh"

using namespace std;
using namespace meth1;

using RR = Node::RR;
using Recs = Node::RecV;

tdiff_t ttr, ttw;

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
    auto tr = time_now();
    auto recs = node->Read( pos, block_size );

#if REUSE_MEM == 1
    RR * rec_copy = new RR[recs.size()];
    for ( size_t i = 0; i < recs.size(); i++ ) {
      rec_copy[i].copy( recs[i] );
    }
    ttr += time_diff<ms>( tr );
    resp.send({ rec_copy, recs.size() });
#else
    ttr += time_diff<ms>( tr );
    resp.send( move( recs ) );
#endif
  }
}

void run( char * fin, char * fout, double block )
{
  BufferedIO_O<File> out( {fout, O_WRONLY | O_CREAT | O_TRUNC,
                                 S_IRUSR | S_IWUSR} );
  auto t0 = time_now();

  // start node
  Node node{fin, "0", true };
  node.Initialize();
  auto t1 = time_now();

  // size
  auto siz = node.Size();
  auto t2 = time_now();

  if ( siz <= 0 ) {
    cout << "Empty file!" << endl;
    return;
  }

  // read + write
  Channel<size_t> req( 0 );
  Channel<Recs> resp( 0 );
  size_t block_size = block * siz;
  thread rthread( reader, &node, block_size, req, resp );

  req.send( 0 );
  for ( size_t i = block_size; i < siz; i += block_size ) {
    auto recs = resp.recv();
    req.send( i ); // send now to overlap with writing.

    auto tw = time_now();
    for ( auto & r : recs ) {
      r.write( out );
    }
    out.flush( true );
    out.io().fsync();
    ttw += time_diff<ms>( tw );
  }

  // last block
  auto recs = resp.recv();
  auto tw = time_now();
  for ( auto & r : recs ) {
    r.write( out );
  }
  out.flush( true );
  out.io().fsync();
  ttw += time_diff<ms>( tw );
  
  // done
  req.close();
  rthread.join();

  auto t3 = time_now();

  // stats
  auto tinit = time_diff<ms>( t1, t0 );
  auto tsize = time_diff<ms>( t2, t1 );
  auto ttota = time_diff<ms>( t3, t0 );

  cout << "-----------------------" << endl;
  cout << "Records " << siz << endl;
  cout << "-----------------------" << endl;
  cout << "Start took " << tinit << "ms" << endl;
  cout << "Size  took " << tsize << "ms" << endl;
  cout << "Read  took " << ttr   << "ms (read + sort)" << endl;
  cout << "Write took " << ttw   << "ms" << endl;
  cout << "Total took " << ttota << "ms" << endl;
  cout << "Writ calls (buf) " << out.write_count() << endl;
  cout << "Writ calls (dir) " << out.io().write_count() << endl;
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

