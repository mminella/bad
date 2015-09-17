
#include <cassert>
#include <vector>

#include "buffered_io.hh"
#include "channel.hh"
#include "file.hh"

#include "record.hh"

#include "client.hh"
#include "cluster.hh"
#include "exception.hh"
#include "priority_queue.hh"
#include "remote_file.hh"

using namespace std;
using namespace meth2;

Cluster::Cluster( vector<Address> nodes, uint64_t chunkSize )
  : clients_{}
  , chunkSize_{chunkSize}
{
  for ( auto & n : nodes ) {
    clients_.push_back( n );
  }
}

Cluster::~Cluster()
{
}

uint64_t Cluster::Size( void )
{
  for ( auto & c : clients_ ) {
    c.sendSize();
  }
  
  uint64_t size = 0;
  for ( auto & c : clients_ ) {
    size += c.recvSize();
  }

  return size;
}

uint64_t
GetLargest(std::vector<Cluster::NodeSplit> &ns)
{
    uint64_t n = 0;

    for (uint64_t i = 0; i < ns.size(); i++) {
	if (ns[n].key.compare(ns[i].key) <= 0) {
	    n = i;
	}
    }

    return n;
}

uint64_t
GetSmallest(std::vector<Cluster::NodeSplit> &ns)
{
    uint64_t n = 0;

    for (uint64_t i = 0; i < ns.size(); i++) {
	if (ns[n].key.compare(ns[i].key) > 0) {
	    n = i;
	}
    }

    return n;
}

bool
SplitDone(std::vector<Cluster::NodeSplit> &ns)
{
    for (uint64_t i = 0; i < ns.size(); i++) {
	for (uint64_t j = 0; j < ns.size(); j++) {
	    //cout << i << "IJ" << j << endl;
	    if (ns[i].key.compare(ns[j].nextKey) > 0)
		return false;
	}
    }

    return true;
}

std::vector<Cluster::NodeSplit>
Cluster::GetSplit(uint64_t n)
{
    uint64_t i;
    std::vector<NodeSplit> ns;

    //auto rmid = IRead(clients_[0], Size(clients_[0]) / 2, 1);
    //auto rsrch = IBSearch(clients_[0], rmid[0]);

    for (i = 0; i < clients_.size(); i++) {
	ns.emplace_back();
	ns[i].clientNo = i;
	ns[i].size = Size(clients_[i]);
	ns[i].start = 0;
	ns[i].end = n - 1;
	ns[i].n = n / clients_.size();
    }
    // Adjust ns[0].n to account for rounding
    ns[0].n += n - (ns[0].n * clients_.size());

    for (i = 0; i < clients_.size(); i++) {
	// Read n and n+1
	vector<RecordLoc> rl = IRead(clients_[i], ns[i].n, 2);
	ns[i].key = rl[0];
	ns[i].nextKey = rl[1];
    }

    /*for (auto &n : ns)
	n.dump();*/

    while (!SplitDone(ns)) {
	uint64_t largest, smallest, step;
	uint64_t newStart, newEnd;

	largest = GetLargest(ns);
	smallest = GetSmallest(ns);

	newStart = IBSearch(clients_[ns[largest].clientNo],
			    ns[smallest].key);
	newEnd = ns[largest].n;
	if (newStart > ns[largest].start)
	    ns[largest].start = newStart;
	if (newEnd < ns[largest].end)
	    ns[largest].end = newEnd;

	newStart = ns[smallest].n;
	newEnd = IBSearch(clients_[ns[smallest].clientNo],
			  ns[largest].key);
	if (newStart > ns[smallest].start)
	    ns[smallest].start = newStart;
	if (newEnd < ns[smallest].end)
	    ns[smallest].end = newEnd;

	uint64_t largestStep = ns[largest].end - ns[largest].start;
	uint64_t smallestStep = ns[smallest].end - ns[smallest].start;
	step = (largestStep > smallestStep) ? smallestStep : largestStep;
    
	step /= 2;
	if (step == 0)
	    step = 1;

	ns[largest].n -= step;
	ns[smallest].n += step;

	for (i = 0; i < clients_.size(); i++) {
	    // Read n and n+1
	    vector<RecordLoc> rl = IRead(clients_[i], ns[i].n, 2);
	    ns[i].key = rl[0];
	    ns[i].nextKey = rl[1];
	}

	/*for (auto &n : ns)
	    n.dump();*/
    }

    for (auto &n : ns)
	n.dump();

    return ns;
}

uint64_t
Cluster::IBSearch( Client &c, RecordLoc &rl )
{
    vector<RecordLoc> tmp;
    uint64_t start = 0;
    uint64_t end = Size(c);
    uint64_t pos = end / 2;

    //cout << "IBSearch" << endl;
    while (start < end) {
	//cout << start << ", " << end << ", " << pos << endl;
	tmp = IRead(c, pos, 1);
	int val = tmp[0].compare(rl);
	if (val > 0) {
	    end = pos;
	    pos = (start + end) / 2;
	} else if (val < 0) {
	    start = pos + 1;
	    pos = (start + end) / 2;
	} else {
	    return pos;
	}
    }
    //cout << "IBSearch" << endl;

    return pos;
}

Record Cluster::ReadFirst( void )
{
  for ( auto & c : clients_ ) {
    c.sendRead( 0, 1 );
  }

  Record min( Rec::MAX );
  for ( auto & c : clients_ ) {
    uint64_t s = c.recvRead();
    if ( s >= 1 ) {
      RecordPtr p = c.readRecord();
      if ( p < min ) {
        min.copy( p );
      }
    }
  }

  return min;
}



void Cluster::Read( uint64_t pos, uint64_t size )
{
  assert(false);
  if ( clients_.size() == 1 ) {
    // optimize for 1 node
    auto & c = clients_.front();
    uint64_t totalSize = Size();
    if ( pos < totalSize ) {
      size = min( totalSize - pos, size );
      for ( uint64_t i = pos; i < pos + size; i += chunkSize_ ) {
        uint64_t n = min( chunkSize_, pos + size - i );
        c.sendRead( i, n );
        auto nrecs = c.recvRead();
        for ( uint64_t j = 0; j < nrecs; j++ ) {
          c.readRecord();
        }
      }
    }
  } else {
    // general n node case
    vector<RemoteFile> files;
    mystl::priority_queue_min<RemoteFile> pq{clients_.size()};
    uint64_t totalSize = Size();
    uint64_t end = min( totalSize, pos + size );

    // prep -- size
    for ( auto & c : clients_ ) {
      RemoteFile f( c, chunkSize_ );
      f.sendSize();
      files.push_back( f );
    }

    // prep -- 1st chunk
    for ( auto & f : files ) {
      f.recvSize();
      f.nextChunk( size );
    }

    // prep -- 1st record
    for ( auto & f : files ) {
      f.nextRecord( size );
      pq.push( f );
    }

    // read to end records
    for ( uint64_t i = 0; i < end; i++ ) {
      RemoteFile f = pq.top();
      pq.pop();
      if ( !f.eof() ) {
        f.nextRecord( size - i );
        pq.push( f );
      }
    }
  }
}

void Cluster::ReadAll( void )
{
  if ( clients_.size() == 1 ) {
    // optimize for 1 node
    auto & c = clients_.front();
    uint64_t size = Size();
    for ( uint64_t i = 0; i < size; i += chunkSize_ ) {
      c.sendRead( i, chunkSize_ );
      auto nrecs = c.recvRead();
      for ( uint64_t j = 0; j < nrecs; j++ ) {
        c.readRecord();
      }
    }
  } else {
    // general n node case
    vector<RemoteFile> files;
    mystl::priority_queue_min<RemoteFile> pq{clients_.size()};
    uint64_t size = Size();

    // prep -- size
    for ( auto & c : clients_ ) {
      RemoteFile f( c, chunkSize_ );
      f.sendSize();
      files.push_back( f );
    }

    // prep -- 1st chunk
    for ( auto & f : files ) {
      f.recvSize();
      f.nextChunk();
    }

    // prep -- 1st record
    for ( auto & f : files ) {
      f.nextRecord();
      pq.push( f );
    }

    // read all records
    for ( uint64_t i = 0; i < size; i++ ) {
      RemoteFile f = pq.top();
      pq.pop();
      if ( !f.eof() ) {
        f.nextRecord();
        pq.push( f );
      }
    }
  }
}

static void writer( File out, Channel<vector<Record>> chn )
{
  static int pass = 0;
  BufferedIO bio( out );
  vector<Record> recs;
  while ( true ) {
    try {
      cout << "wait write!" << endl;
      recs = move( chn.recv() );
      cout << "got write!" << endl;
    } catch ( const std::exception & e ) {
      print_exception( e );
      break;
    }
    auto t0 = time_now();
    for ( auto const & r : recs ) {
      r.write( bio );
    }
    bio.flush( true );
    out.fsync();
    auto twrite = time_diff<ms>( t0 );
    cout << "write, " << pass++ << ", " << twrite << endl;
  }
}

static const size_t WRITE_BUF = 1048576; // 100MB * 2
static const size_t WRITE_BUF_N = 2;

void Cluster::WriteAll( File out )
{
  if ( clients_.size() == 1 ) {
    // optimize for 1 node
    auto & c = clients_.front();
    uint64_t size = Size();
    for ( uint64_t i = 0; i < size; i += chunkSize_ ) {
      c.sendRead( i, chunkSize_ );
      auto nrecs = c.recvRead();
      for ( uint64_t j = 0; j < nrecs; j++ ) {
        RecordPtr rec = c.readRecord();
        rec.write( out );
      }
    }
  } else {
    // general n node case
    vector<RemoteFile> files;
    mystl::priority_queue_min<RemoteFile> pq{clients_.size()};
    uint64_t size = Size();

    Channel<vector<Record>> chn( WRITE_BUF_N - 1 );
    thread twriter( writer, move( out ), chn );

    // prep -- size
    for ( auto & c : clients_ ) {
      RemoteFile f( c, chunkSize_ );
      f.sendSize();
      files.push_back( f );
    }

    // prep -- 1st chunk
    for ( auto & f : files ) {
      f.recvSize();
      f.nextChunk();
    }

    // prep -- 1st record
    for ( auto & f : files ) {
      f.nextRecord();
      pq.push( f );
    }

    // read all records
    vector<Record> recs;
    recs.reserve( WRITE_BUF );
    for ( uint64_t i = 0; i < size and pq.size() > 0; i++ ) {
      RemoteFile f = pq.top();
      pq.pop();
      recs.emplace_back( f.curRecord() );
      if ( !f.eof() ) {
        f.nextRecord();
        pq.push( f );
      }
      if ( recs.size() >= WRITE_BUF ) {
        chn.send( move( recs ) );
        recs = vector<Record>();
      }
    }

    // wait for write to finish
    if ( recs.size() > 0 ) {
      chn.send( move( recs ) );
    }
    chn.waitEmpty();
    chn.close();
    twriter.join();
  }
}

uint64_t
Cluster::Size( Client &c )
{
    c.sendSize();
    return c.recvSize();
}

std::vector<RecordLoc>
Cluster::IRead( Client &c, uint64_t pos, uint64_t size )
{
    uint64_t nrecs;
    std::vector<RecordLoc> rl;

    c.sendIRead( pos, size );
    nrecs = c.recvIRead();

    rl.reserve(nrecs);

    for ( uint64_t i = 0; i < nrecs; i++ ) {
        rl.emplace_back(c.readIRecord());
    }

    return rl;
}

