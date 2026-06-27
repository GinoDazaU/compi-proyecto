fn main() {
    let n: usize = 1000000;
    let mut s = vec![1i64; n];

    s[0] = 0;
    s[1] = 0;

    let mut i = 2;
    while i < n {
        if s[i] == 1 {
            let mut j = i * i;
            while j < n {
                s[j] = 0;
                j += i;
            }
        }
        i += 1;
    }

    let mut count: i64 = 0;
    for i in 2..n {
        count += s[i];
    }
    println!("{}", count);
}
