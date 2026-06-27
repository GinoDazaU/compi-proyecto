package main

import "fmt"

func quicksort(a []int64, lo, hi int64) {
	if lo >= hi {
		return
	}
	pivot := a[hi]
	i := lo - 1
	for j := lo; j < hi; j++ {
		if a[j] < pivot {
			i++
			a[i], a[j] = a[j], a[i]
		}
	}
	p := i + 1
	a[p], a[hi] = a[hi], a[p]
	quicksort(a, lo, p-1)
	quicksort(a, p+1, hi)
}

func main() {
	var n int64 = 1000000
	a := make([]int64, n)
	for i := int64(0); i < n; i++ {
		a[i] = (i*1103515245 + 12345) % 1000000
	}

	quicksort(a, 0, n-1)

	var chk int64 = 0
	for i := int64(0); i < n; i++ {
		chk += a[i] * i
	}
	fmt.Println(chk)
}
