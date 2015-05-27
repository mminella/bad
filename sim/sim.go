package main

import "fmt"
import "unsafe"

func main() {
  imp := NewImp1()

  imp.nodeStartup()
  fmt.Printf("Startup Cost:\t\t%v\n", imp.node.now)

  imp.nodeNextRecord()
  fmt.Printf("NextRecord Cost:\t%v\n", imp.node.now)
}

type imp1 struct {
  node *Node
  file *File
  last *RecordOff
}

func NewImp1() *imp1 {
  return &imp1 {
    node: NewNode(),
  }
}

func (n *imp1) nodeStartup() {
  n.file = NewFile(n.node, DATA)
  n.last = &RecordOff{ RecordMIN(), 0 }
}

func (n *imp1) nodeNextRecord() {
  n.file.Rewind()
  n.last = linearScan(n.file, n.last)
}

func linearScan(in *File, after *RecordOff) *RecordOff {
  min := &RecordOff{ RecordMAX(), 0 }

  for i := uint64(1); ; i++  {
    rec := in.Read( RECORD_SIZE )
    if in.EOF() {
      break;
    }

    next := (*Record)(unsafe.Pointer(&rec[0]))

    cmp := next.compare(&after.Record)
    if (cmp == EQ || cmp == GT) && next.lessThan(&min.Record) {
      // account for duplicate keys
      if (cmp == GT || after.offset < i ) {
        min = &RecordOff{ *next, i }
      }
    }
  }

  return min
}

