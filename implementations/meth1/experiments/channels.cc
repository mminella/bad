/**
 * Test out our channels implementation.
 */
#include <iostream>
#include <mutex>
#include <thread>

#include "channel.hh"

using namespace std;

// mutex mtx;

void thread_a( int id, Channel<int> c )
{
  for ( int i = 0; i < 20; i++ ) {
    c.send( id );
    // lock_guard<mutex> lck( mtx );
    // cout << "[" << id << "] Sent" << endl;
  }
}

void thread_b( int id, Channel<int> c )
{
  (void) id;
  for ( int i = 0; i < 20; i++ ) {
    int tid = c.recv();
    (void) tid;
    // lock_guard<mutex> lck( mtx );
    // cout << "[" << id << "] Recv: " << tid << endl;
  }
}

int main( void )
{
  for ( int i = 0; i < 50; i++ ) {
    Channel<int> async( 5 );

    thread a1 ( thread_a, 0, async );
    thread a2 ( thread_a, 1, async );
    thread b1 ( thread_b, 2, async );
    thread b2 ( thread_b, 3, async );

    a1.join();
    a2.join();
    b1.join();
    b2.join();
    // cout << "Done async..." << endl;

    Channel<int> sync( 0 );

    thread a3 ( thread_a, 0, sync );
    thread a4 ( thread_a, 1, sync );
    thread b3 ( thread_b, 2, sync );
    thread b4 ( thread_b, 3, sync );

    a3.join();
    a4.join();
    b3.join();
    b4.join();
    // cout << "Done sync..." << endl;
  }
  return EXIT_SUCCESS;
}

