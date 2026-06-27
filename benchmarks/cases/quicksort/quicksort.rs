fn quicksort(a: &mut [i64], lo: i64, hi: i64) {
    if lo >= hi {
        return;
    }
    let pivot = a[hi as usize];
    let mut i = lo - 1;
    for j in lo..hi {
        if a[j as usize] < pivot {
            i += 1;
            a.swap(i as usize, j as usize);
        }
    }
    let p = i + 1;
    a.swap(p as usize, hi as usize);
    quicksort(a, lo, p - 1);
    quicksort(a, p + 1, hi);
}

fn main() {
    let n: i64 = 1000000;
    let mut a = vec![0i64; n as usize];
    for i in 0..n {
        a[i as usize] = (i * 1103515245 + 12345) % 1000000;
    }

    quicksort(&mut a, 0, n - 1);

    let mut chk: i64 = 0;
    for i in 0..n {
        chk += a[i as usize] * i;
    }
    println!("{}", chk);
}
