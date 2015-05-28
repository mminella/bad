package main

import (
	"fmt"

	"bad/settings"
	"bad/simulator"
	"bad/sort"
)

func main() {
	imp := NewImp1()

	imp.nodeStartup()
	fmt.Printf("Startup Cost:\t\t%v\n", imp.node.GetTime())

	imp.nodeNextRecord()
	fmt.Printf("NextRecord Cost:\t%v\n", imp.node.GetTime())
}

type imp1 struct {
	node *simulator.Node
	file *simulator.File
	last *sort.RecordOff
}

func NewImp1() *imp1 {
	return &imp1{node: simulator.NewNode()}
}

func (n *imp1) nodeStartup() {
	n.file = n.node.OpenDataFile()
	n.last = &sort.RecordOff{sort.RecordMIN(), 0}
}

func (n *imp1) nodeNextRecord() {
	n.file.Rewind()
	n.last = linearScan(n.file, n.last)
}

func linearScan(in *simulator.File, after *sort.RecordOff) *sort.RecordOff {
	min := &sort.RecordOff{sort.RecordMAX(), 0}

	for i := uint64(1); ; i++ {
		rec := in.Read(settings.RECORD_SIZE)
		if in.EOF() {
			break
		}

		next := sort.UnsafeRecord(rec)

		cmp := next.Compare(&after.Record)
		if (cmp == sort.EQ || cmp == sort.GT) && next.LessThan(&min.Record) {
			// account for duplicate keys
			if cmp == sort.GT || after.Offset < i {
				min = &sort.RecordOff{*next, i}
			}
		}
	}

	return min
}
