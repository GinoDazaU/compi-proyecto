fn main() {
    let n: i64 = 10000000;
    let mut sum: f64 = 0.0;
    let mut sign: f64 = 1.0;

    for k in 0..n {
        let denom = 2.0 * k as f64 + 1.0;
        sum = sum + sign / denom;
        sign = -sign;
    }

    let pi = sum * 4.0;
    println!("{:.6}", pi);
}
