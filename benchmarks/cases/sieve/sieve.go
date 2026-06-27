package main

import "fmt"

func main() {
	n := 1000000
	s := make([]int64, n)

	for i := 0; i < n; i++ {
		s[i] = 1
	}
	s[0] = 0
	s[1] = 0

	for i := 2; i < n; i++ {
		if s[i] == 1 {
			for j := i * i; j < n; j += i {
				s[j] = 0
			}
		}
	}

	var count int64 = 0
	for i := 2; i < n; i++ {
		count += s[i]
	}
	fmt.Println(count)
}
