package main

import "fmt"

func main() {
	var n int64 = 10000000
	sum := 0.0
	sign := 1.0

	for k := int64(0); k < n; k++ {
		denom := 2.0*float64(k) + 1.0
		sum = sum + sign/denom
		sign = -sign
	}

	pi := sum * 4.0
	fmt.Printf("%.6f\n", pi)
}
