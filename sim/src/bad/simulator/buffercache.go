package simulator

type bufferCache struct {
	disk  *disk
	cache *lruCache 
}

func newBufferCache(disk *disk, cap int) *bufferCache {
	return &bufferCache {
		disk: disk,
		cache: newLRUCache(cap),
	}
}

func (bc *bufferCache) readBlock(b blockAddr) block {
	blk := bc.cache.get(b)
	if blk != nil {
		return blk
	}

	blk = bc.disk.readBlock(b)
	bc.cache.set(b, blk)
	return blk
}
