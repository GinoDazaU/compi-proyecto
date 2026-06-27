package main

import "fmt"

func main() {
	var w int64 = 600
	var h int64 = 600
	var maxIter int64 = 256
	var total int64 = 0

	for py := int64(0); py < h; py++ {
		for px := int64(0); px < w; px++ {
			x0 := float64(px) * 1.0 / float64(w) * 3.5 - 2.5
			y0 := float64(py) * 1.0 / float64(h) * 2.0 - 1.0
			x := 0.0
			y := 0.0
			var iter int64 = 0
			for iter < maxIter && x*x+y*y < 4.0 {
				xt := x*x - y*y + x0
				y = 2.0*x*y + y0
				x = xt
				iter++
			}
			total += iter
		}
	}

	fmt.Println(total)
}
