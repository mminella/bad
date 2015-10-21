
#include <algorithm>
#include <cassert>
#include <string>
#include <vector>
#include <chrono>
#include <utility>
#include <fstream>

#include "buffered_io.hh"
#include "file.hh"
#include "overlapped_rec_io.hh"
#include "timestamp.hh"

#include "record.hh"

#define MIN_NRECS 1000

using namespace std;

vector<File> files;
vector<RecordLoc> recs_;
fstream results;

void
measure(uint64_t nrecs_per_disk)
{
    cout << "size: " << nrecs_per_disk << " recs/disk" << endl;

    // Load records
    auto start = time_now();
    for (size_t d = 0; d < files.size(); d++) {
	OverlappedRecordIO<Rec::SIZE> cio(files[d]);

	cio.rewind();
	for (uint64_t i = 0; i < nrecs_per_disk; i++) {
	    const uint8_t *rec = (const uint8_t *)cio.next_record();
	    recs_.emplace_back(/*key*/rec,
			       /*loc*/i * Rec::SIZE + Rec::KEY_LEN,
			       /*host*/0,
			       /*disk*/d);
	}
    }

    auto loadTime = time_diff<ms>(start);
    cout << "load: " << loadTime << "mS" << endl;

    // Sort
    start = time_now();
    rec_sort(recs_.begin(), recs_.end());

    auto sortTime = time_diff<ms>(start);
    cout << "sort: " << sortTime << "mS" << endl;

    results << nrecs_per_disk * files.size()
	    << ", " << loadTime
	    << ", " << sortTime
	    << endl;

    recs_.clear();

    return;
}

int
main(int argc, const char *argv[])
{
    int i;

    if (argc < 2) {
	cout << "sortperf FILE1 ... FILEN" << endl;
	return 1;
    }

    results.open("sortperf.csv", std::ios::out | std::ios::trunc);
    results << "recs, loadtime, sorttime" << endl;

    for (i = 1; i < argc; i++) {
	cout << "input " << argv[i] << endl;
	files.emplace_back(argv[i], O_RDONLY);
    }

    uint64_t maxRecs = files[0].size() / Rec::SIZE;
    cout << "maxRecs " << maxRecs << endl;

    recs_.reserve(maxRecs * files.size());
    cout << "reserve complete" << endl;

    uintptr_t first = (uintptr_t)(&recs_[0]);
    uintptr_t second = (uintptr_t)(&recs_[1]);
    cout << "array size " << second - first << " B/entry" << endl;

    for (uint64_t nrecs = MIN_NRECS; nrecs <= maxRecs; nrecs *= 10) {
	measure(nrecs);
    }
}

