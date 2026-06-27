fn main() {
    let mut a = [0i64; 10000];

    for i in 0..10000i64 {
        a[i as usize] = (i * 31 + 7) % 1000;
    }

    for i in 0..9999 {
        for j in 0..(9999 - i) {
            if a[j] > a[j + 1] {
                let tmp = a[j];
                a[j] = a[j + 1];
                a[j + 1] = tmp;
            }
        }
    }

    let mut chk: i64 = 0;
    for i in 0..10000 {
        chk += a[i] * i as i64;
    }
    println!("{}", chk);
}
