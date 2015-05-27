package main

import "encoding/binary"
import "sync"
import "time"

type File struct {
  node       *Node
  lock       sync.Mutex
  fpos       uint64
  cache      []byte
  cacheStart block_offset
  cacheEnd   block_offset
  size       uint64
  eof        bool
}

func NewFile(n *Node, size uint64) *File {
  return &File{node: n, size: size}
}

func (f *File) Lock() { f.lock.Lock() }
func (f *File) Unlock() { f.lock.Unlock() }

func (f *File) inCache(offset, size uint64) bool {
  if uint64(f.cacheStart) <= offset && offset + size <= uint64(f.cacheEnd) {
    return true
  }
  return false
}

func (f *File) cacheRead(size uint64) []byte {
  start := f.fpos - uint64(f.cacheStart)
  b := f.cache[start:start+size]
  f.fpos = min(f.fpos + size, f.size)
  return b
}

func (f *File) rawRead(size uint64) {
  pos := block_offset(f.fpos).align()
  f.cacheStart = pos
  f.cache = []byte{}
  for ; uint64(pos) < f.fpos + size; pos += IO_BLOCK {
    f.cache = append(f.cache, f.node.readBlock(pos)...)
  }
  f.cacheEnd = pos
}

func (f *File) Read(size uint64) []byte {
  f.Lock()
  defer f.Unlock()

  if f.fpos + size < f.fpos {
    panic("read wrapped around! used smaller size")
  } else if f.fpos == f.size {
    f.eof = true
    return []byte{}
  }

  if f.inCache(f.fpos, size) {
    return f.cacheRead(size)
  }
  
  f.rawRead(size)
  return f.cacheRead(size)
}

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

type Node struct {
  lock sync.Mutex
  now time.Duration
}

func NewNode() *Node {
  return &Node{}
}

func (n *Node) Lock() { n.lock.Lock() }
func (n *Node) Unlock() { n.lock.Unlock() }

func (n *Node) readBlock(b block_offset) []byte {
  n.Lock()
  defer n.Unlock()
  n.now += time.Second / IOPS
  return b.generateBlock()
}

type block_offset uint64

func (b block_offset) align() block_offset {
  return b / IO_BLOCK * IO_BLOCK
}

func (b block_offset) generateBlock() []byte {
  firstKey := b / RECORD_SIZE * RECORD_SIZE
  if firstKey < b {
    firstKey += RECORD_SIZE
  }

  block := [IO_BLOCK]byte{}

  for k := firstKey; k < b + IO_BLOCK; k += RECORD_SIZE {
    binary.PutUvarint(block[k - b:], uint64(k))
  }

  return block[:]
}
