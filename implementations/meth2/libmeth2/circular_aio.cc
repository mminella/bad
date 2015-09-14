
#include <cassert>

#include <vector>
#include <atomic>
#include <thread>
#include <mutex>

#include "node.hh"
#include "circular_aio.hh"

#define AIO_MAX_THREADS_PER_DISK	16

using namespace std;

namespace meth2
{

Circular_AIO::Circular_AIO(vector<File> &dev, vector<RecordLoc> &recs_)
    : requestExit(false),
      cmd(),
      result(),
      threads(),
      io_(dev),
      recs_(recs_),
      out(nullptr),
      posMutex(),
      pos(0),
      start(0),
      size(0)
{
    std::atomic_store(&requestExit, false);

    for (size_t i = 0; i < (dev.size() * AIO_MAX_THREADS_PER_DISK); i++) {
	threads.emplace_back(&Circular_AIO::ioThread, this);
    }
}

Circular_AIO::~Circular_AIO()
{
    int i;

    std::atomic_store(&requestExit, true);

    for (i = 0; i < (int)threads.size(); i++) {
	cmd.send(0);
    }

    for (auto &t : threads) {
	t.join();
    }
}

void
Circular_AIO::begin(Node::RecV *buf, uint64_t pos, uint64_t size)
{
    int i;

    this->out = buf;
    this->pos = 0;
    this->start = pos;
    this->size = size;

    // Wakeup
    for (i = 0; i < (int)threads.size(); i++) {
	cmd.send(1);
    }
}

void
Circular_AIO::wait()
{
    int i;
    int failure = 1;

    for (i = 0; i < (int)threads.size(); i++) {
	int status = result.recv();
	if (status != 1) {
	    failure = status;
	}
    }

    assert(failure == 1);
}


void
Circular_AIO::ioProcess()
{
    while (true) {
	uint64_t off;

	// Check and update position
	{
	    std::lock_guard<std::mutex> lock(posMutex);

	    if (pos >= size)
		return;

	    off = pos++;
	}

	uint8_t buf[Rec::VAL_LEN];
	RecordLoc &r = recs_[start+off];

	io_[r.disk()].pread_all((char *)&buf, Rec::VAL_LEN, r.loc());

	(*out)[off] = Node::RR(recs_[off], buf);
    }
}

void
Circular_AIO::ioThread()
{
    while (true) {
	int command = cmd.recv();
	if (requestExit)
	    break;

	ioProcess();

	result.send(1);
    }
}

}

