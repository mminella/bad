package implementation

import (
	"bad/sort"

	"time"
)

// Implementation defines the interface (for a logically sorted file) that any
// particular implementation is meant to provide to a client.
type Implementation interface {
	// Initialize initializes the file system.
	Initialize()

	// Read returns the continguous subset of records that occur at the specified
	// position in the file.
	Read(pos, size uint) []sort.Record

	// ReadAfter returns the next 'size' records that occur immediately after the
	// key specified. This is a strictly greater than comparison, so any
	// duplicates of 'key' will be passed over.
	ReadAfter(key sort.Key, size uint) []sort.Record
	
	// Size returns the number of record in the 'file'.
	Size() uint
}

// Metrics defines how each implementation will be evaluated.
type Metrics interface {
	// Startup returns the time to initialize the file system (the time before a
	// client can start issuing requests).
	Startup() time.Duration

	// FirstRecord returns the time to retrieve the first record from the file
	// system.
	FirstRecord() time.Duration

	// RandomRecord returns the time to retrieve a random record (at a specified
	// position) from the file system.
	RandomRecord(pos uint) time.Duration

	// AllRecords returns the time to retrieve the complete file.
	AllRecords() time.Duration
}
