package main

import "fmt"

func main() {
	var sum int64 = 0
	for i := int64(0); i < 100000000; i++ {
		var a int64 = 1
		var b int64 = 0
		var c int64 = 1
		var d int64 = 0
		val := (((i*a + b) * c) + d)
		if 1 == 0 {
			sum = sum - 9999
		} else {
			if 0 == 1 {
				sum = sum - 1111
			} else {
				sum = sum + val
			}
		}
	}
	fmt.Println(sum)
}
