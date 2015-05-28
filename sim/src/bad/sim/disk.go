package sim

import (
	"time"

	"bad/settings"
)

type block []byte

type blockAddr uint64

func (b blockAddr) align() blockAddr {
	return b / settings.IO_BLOCK * settings.IO_BLOCK
}

type disk struct {
	clock Clock
	gen   dataGenerator
}

func newDisk(clock Clock, gen dataGenerator) *disk {
	return &disk{clock, gen}
}

func (d *disk) readBlock(b blockAddr) block {
	d.clock.AddTime(time.Second / settings.IOPS)
	return d.gen(b)
}
