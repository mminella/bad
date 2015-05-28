package sim

import (
	"sync"

	"bad/settings"
	"bad/utils"
)

type File struct {
	bcache     *bufferCache
	lock       sync.Mutex
	fpos       uint64
	size       uint64
	eof        bool
}

func newFile(bc *bufferCache, size uint64) *File {
	return &File{bcache: bc, size: size}
}

func (f *File) Lock()   { f.lock.Lock() }
func (f *File) Unlock() { f.lock.Unlock() }

func (f *File) EOF() bool {
	f.Lock()
	defer f.Unlock()

	return f.eof
}

func (f *File) Rewind() {
	f.Lock()
	defer f.Unlock()

	f.eof = false
	f.fpos = 0
}

func (f *File) Read(size uint64) []byte {
	f.Lock()
	defer f.Unlock()

	if f.fpos+size < f.fpos {
		panic("read wrapped around! used smaller size")
	} else if f.fpos == f.size {
		f.eof = true
		return []byte{}
	}

	b := []byte{}
	for pos := blockAddr(f.fpos).align();
			uint64(pos) < f.fpos+size;
			pos += settings.IO_BLOCK {
		b = append(b, f.bcache.readBlock(pos)...)
	}

	f.fpos = utils.Min(f.fpos+size, f.size)

	return b
}
