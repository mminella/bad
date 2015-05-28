package sim

import (
	"encoding/binary"

	"bad/settings"
)

type dataGenerator func(blockAddr) block

/* Uniform data generator (no unique keys) */
func uniformDataGen(addr blockAddr) block {
	firstKey := addr / settings.RECORD_SIZE * settings.RECORD_SIZE
	if firstKey < addr {
		firstKey += settings.RECORD_SIZE
	}

	block := [settings.IO_BLOCK]byte{}

	for k := firstKey; k < addr + settings.IO_BLOCK; k += settings.RECORD_SIZE {
		binary.PutUvarint(block[k-addr:], uint64(k))
	}

	return block[:]
}
