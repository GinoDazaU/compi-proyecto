package main

import "fmt"

func main() {
	var a [10000]int64

	for i := int64(0); i < 10000; i++ {
		a[i] = (i*31 + 7) % 1000
	}

	for i := 0; i < 9999; i++ {
		for j := 0; j < 9999-i; j++ {
			if a[j] > a[j+1] {
				tmp := a[j]
				a[j] = a[j+1]
				a[j+1] = tmp
			}
		}
	}

	var chk int64 = 0
	for i := 0; i < 10000; i++ {
		chk += a[i] * int64(i)
	}
	fmt.Println(chk)
}
