package sort

import (
	"bytes"
	"unsafe"

	"bad/settings"
)

type Key [settings.KEY_SIZE]uint8
type Val [settings.VAL_SIZE]uint8

type Record struct {
	Key Key
	Val Val
}

type Ord int

const (
	LT Ord = -1
	EQ Ord = 0
	GT Ord = 1
)

type RecordOff struct {
	Record
	Offset uint64
}

func RecordMIN() Record {
	return Record{}
}

func UnsafeRecord(bytes []byte) *Record {
	return (*Record)(unsafe.Pointer(&bytes[0]))
}

func RecordMAX() Record {
	k := bytes.Repeat([]uint8{0xFF}, settings.KEY_SIZE)
	r := Record{}
	copy(r.Key[:], k)
	return r
}

func (r *Record) LessThan(o *Record) bool {
	if bytes.Compare(r.Key[:], o.Key[:]) < 0 {
		return true
	}
	return false
}

func (r *Record) LessThanEq(o *Record) bool {
	if bytes.Compare(r.Key[:], o.Key[:]) <= 0 {
		return true
	}
	return false
}

func (r *Record) Compare(o *Record) Ord {
	return Ord(bytes.Compare(r.Key[:], o.Key[:]))
}