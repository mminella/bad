
#ifndef __CIRCULAR_AIO_H__
#define __CIRCULAR_AIO_H__

namespace meth2
{

class Circular_AIO {
public:
    struct Buffer {
	void *buf;
	uint64_t len;
    };
    Circular_AIO(std::vector<File> &dev, std::vector<RecordLoc> &recs_);
    ~Circular_AIO();
    void begin(Node::RecV *buf, uint64_t start, uint64_t size);
    void wait();
private:
    void ioProcess();
    void ioThread();
    std::atomic<bool> requestExit;
    Channel<int> cmd;
    Channel<int> result;
    std::vector<std::thread> threads;
    // IO Device
    std::vector<File> &io_;
    // Sorted Records
    std::vector<RecordLoc> &recs_;
    // Output
    Node::RecV *out;
    std::mutex posMutex;
    uint64_t pos;
    uint64_t start;
    uint64_t size;
};

}

#endif /* __CIRCULAR_AIO_H__ */

