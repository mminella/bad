package sim

import (
	"bad/settings"
)

type bufferCache struct {
	node  *Node
	cache *lruCache 
}

func newBufferCache(n *Node) *bufferCache {
	return &bufferCache {
		node: n,
		cache: newLRUCache(settings.BUF_CACHE),
	}
}

func (bc *bufferCache) readBlock(b blockAddr) block {
	blk := bc.cache.get(b)
	if blk != nil {
		return blk
	}

	blk = bc.node.readBlock(b)
	bc.cache.set(b, blk)
	return blk
}

