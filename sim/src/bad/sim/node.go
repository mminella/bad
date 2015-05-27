package sim

import (
	"sync"
	"time"
)

type Node struct {
	lock sync.Mutex
	now  time.Duration
}

func NewNode() *Node {
	return &Node{}
}

func (n *Node) GetRunTime() time.Duration {
	return n.now
}

func (n *Node) Lock()   { n.lock.Lock() }
func (n *Node) Unlock() { n.lock.Unlock() }
