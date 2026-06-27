package main

import "fmt"

func main() {
	var n int64 = 1000000
	var total int64 = 0

	for i := int64(1); i <= n; i++ {
		x := i
		for x != 1 {
			if x%2 == 0 {
				x = x / 2
			} else {
				x = 3*x + 1
			}
			total++
		}
	}

	fmt.Println(total)
}
