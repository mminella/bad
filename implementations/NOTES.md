# Implementation Notes

## File System IO

We need to develpo a good approach to disk I/O. Some options:

* `io_setup (2)` -- Linux Kernel API for async IO.
* non-blocking IO -- ? Does linux support non-blocking I/O with select, poll
  and epoll?
* threads -- Use blocking IO but in separate threads.

### Libraries

* aio -- http://lse.sourceforge.net/io/aio.html (wrapper around `io_setup (2)`)
* ACE -- http://www.cs.wustl.edu/~schmidt/ACE.html
* Asio -- http://think-async.com/Asio/

Not clear to me if these libraries support file-system IO, or just network:

* Libuv -- https://github.com/libuv/libuv
* Libevent -- http://libevent.org/

## Overviews

http://www.kegel.com/c10k.html
https://nick-black.com/dankwiki/index.php/Fast_UNIX_Servers

