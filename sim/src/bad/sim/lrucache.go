package sim

import (
	"container/list"
)

type lruCache struct {
	limit int
	cache map[blockAddr]buffer
	lru   *list.List
}

type buffer struct {
	blk block
	ptr *list.Element
}

func newLRUCache(capacity int) *lruCache {
	return &lruCache {
		limit: capacity,
		cache: make(map[blockAddr]buffer),
		lru:   list.New(),
	}
}

func (c *lruCache) get(addr blockAddr) block {
	buf, ok := c.cache[addr]
	if !ok { return nil }
	c.promote(buf)
	return buf.blk
}

func (c *lruCache) set(addr blockAddr, blk block) {
	buf, exists := c.cache[addr]
	if exists {
		buf.blk = blk
		c.promote(buf)
		return
	}

	if c.lru.Len() >= c.limit {
		c.prune()
	}

	e := c.lru.PushFront(addr)
	c.cache[addr] = buffer{ blk, e }
}

func (c *lruCache) promote(buf buffer) {
	c.lru.MoveToFront(buf.ptr)
}

func (c *lruCache) prune() {
	elm  := c.lru.Back()
	addr := c.lru.Remove(elm).(blockAddr)
	delete(c.cache, addr)
}
