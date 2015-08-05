#include <ctime>
#include <sys/time.h>
#include <unistd.h>

#include "timestamp.hh"
#include "exception.hh"

/* Program start time */
const clk::time_point tstart = time_now();

