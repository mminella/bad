package sim

import (
	"encoding/binary"
	"time"

	"bad/settings"
)

func (n *Node) readBlock(b blockAddr) block {
	n.Lock()
	defer n.Unlock()
	n.now += time.Second / settings.IOPS
	return b.generateBlock()
}

type block []byte

type blockAddr uint64

func (b blockAddr) align() blockAddr {
	return b / settings.IO_BLOCK * settings.IO_BLOCK
}

func (b blockAddr) generateBlock() block {
	firstKey := b / settings.RECORD_SIZE * settings.RECORD_SIZE
	if firstKey < b {
		firstKey += settings.RECORD_SIZE
	}

	block := [settings.IO_BLOCK]byte{}

	for k := firstKey; k < b+settings.IO_BLOCK; k += settings.RECORD_SIZE {
		binary.PutUvarint(block[k-b:], uint64(k))
	}

	return block[:]
}
