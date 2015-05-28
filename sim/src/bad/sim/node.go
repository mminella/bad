package sim

import (
	"sync"
	"time"

	"bad/settings"
)

type Clock interface {
	AddTime(t time.Duration)
	GetTime() time.Duration
}

type Node struct {
	lock sync.Mutex
	now  time.Duration
	buff *bufferCache
	disk *disk
	data *File
	open bool
}

func NewNode() *Node {
	n := &Node{}
	n.disk = newDisk(n, uniformDataGen)
	n.buff = newBufferCache(n.disk, settings.BUF_CACHE)
	n.data = newFile(n.buff, settings.DATA)
	return n
}

func (n *Node) Lock()   { n.lock.Lock() }
func (n *Node) Unlock() { n.lock.Unlock() }

func (n *Node) GetTime() time.Duration {
	return n.now
}

func (n *Node) AddTime(t time.Duration) {
	n.Lock()
	defer n.Unlock()
	n.now += t
}

func (n *Node) OpenDataFile() *File {
	n.Lock()
	defer n.Unlock()

	if n.open {
		panic("Tried to open data file twice! unsupported")
	}
	n.open = true
	return n.data
}
