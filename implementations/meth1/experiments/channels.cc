/**
 * Test out our channels implementation.
 */
#include <iostream>
#include <mutex>
#include <vector>
#include <thread>

#include "channel.hh"
#include "record.hh"

using namespace std;

/* Class to debug copy vs move constructors */
class C
{
public:
  int c_;

  C(void) : c_{0} {};
  explicit C(int c) : c_{c} {}
  C(const C & c) : c_{c.c_} { cout << "C:copy " << c_ << endl; }

  C & operator=( const C & c ) {
    if ( this != &c ) {
      c_ = c.c_;
      cout << "C:=copy " << c_ << endl;
    }
    return *this;
  }

  C( C && c ) : c_{c.c_} {
    c.c_ = 0;
    cout << "C:move " << c_ << endl;
  }

  C & operator=( C && c ) {
    if ( this != &c ) {
      c_ = c.c_;
      c.c_ = 0;
      cout << "C:=move " << c_ << endl;
    }
    return *this;
  }

  ~C() { cout << "C:dtor " << c_ << endl; }
};

ostream & operator<<( ostream & o, const C & c )
{
  return o << c.c_;
}

void thread_a( int id, Channel<int> c )
{
  for ( int i = 0; i < 20; i++ ) {
    c.send( id );
  }
}

void thread_b( int id, Channel<int> c )
{
  (void) id;
  for ( int i = 0; i < 20; i++ ) {
    c.recv();
  }
}

void test_async_channels( void )
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
  }
}

void test_sync_channels( void )
{
  for ( int i = 0; i < 50; i++ ) {
    Channel<int> sync( 0 );

    thread a3 ( thread_a, 0, sync );
    thread a4 ( thread_a, 1, sync );
    thread b3 ( thread_b, 2, sync );
    thread b4 ( thread_b, 3, sync );

    a3.join();
    a4.join();
    b3.join();
    b4.join();
  }
}

void test_copy_move( void )
{
  Channel<C> chn;

  C c1( 1 ), c2( 2 );
  cout << "c1: " << c1 << ", c2: " << c2 << endl;

  chn.send( move( c1 ) );
  c2 = chn.recv();
  cout << "c1: " << c1 << ", c2: " << c2 << endl;

  cout << "-------" << endl;
  chn.send( move( c2 ) );
  C c4 = chn.recv();

  thread t ( [&] {
    chn.send( c1 );
  } );
  c2 = chn.recv();
  cout << "c1: " << c1 << ", c2: " << c2 << endl;

  t.join();
}

void test_vec_records( void )
{
  using R = string;
  char recStr[Rec::SIZE];
  Channel<vector<R>> chn;
  vector<R> recs1;

  recs1.emplace_back( recStr );
  recs1.emplace_back( recStr );

  thread t ( [&] {
    chn.send( recs1 );
  } );
  chn.recv();

  t.join();
}

void test_basic_chan( void )
{
  Channel<int> chn;
  int x = 1;
  chn.send( x );
  chn.recv();
}

int main( void )
{
  test_async_channels();
  test_sync_channels();
  test_copy_move();
  test_vec_records();
  test_basic_chan();

  return EXIT_SUCCESS;
}

