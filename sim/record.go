package main

import "bytes"

type Record struct {
  key [KEY_SIZE]uint8
  val [VAL_SIZE]uint8
}

type Ord int

const (
  LT Ord = -1
  EQ Ord = 0
  GT Ord = 1
)

type RecordOff struct {
  Record
  offset uint64
}

func RecordMIN() Record {
  return Record{}
}

func RecordMAX() Record {
  k := bytes.Repeat( []uint8{0xFF}, KEY_SIZE )
  r := Record{}
  copy(r.key[:], k)
  return r
}

func (r *Record) lessThan(o *Record) bool {
  if (bytes.Compare(r.key[:], o.key[:]) < 0) {
    return true
  }
  return false
}

func (r *Record) lessThanEq(o *Record) bool {
  if (bytes.Compare(r.key[:], o.key[:]) <= 0) {
    return true
  }
  return false
}

func (r *Record) compare(o *Record) Ord {
  return Ord(bytes.Compare(r.key[:], o.key[:]))
}
