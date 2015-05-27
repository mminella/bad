package sim

import "encoding/binary"
import "time"

import set "bad/settings"

func (n *Node) readBlock(b block_offset) []byte {
	n.Lock()
	defer n.Unlock()
	n.now += time.Second / set.IOPS
	return b.generateBlock()
}

type block_offset uint64

func (b block_offset) align() block_offset {
	return b / set.IO_BLOCK * set.IO_BLOCK
}

func (b block_offset) generateBlock() []byte {
	firstKey := b / set.RECORD_SIZE * set.RECORD_SIZE
	if firstKey < b {
		firstKey += set.RECORD_SIZE
	}

	block := [set.IO_BLOCK]byte{}

	for k := firstKey; k < b+set.IO_BLOCK; k += set.RECORD_SIZE {
		binary.PutUvarint(block[k-b:], uint64(k))
	}

	return block[:]
}
