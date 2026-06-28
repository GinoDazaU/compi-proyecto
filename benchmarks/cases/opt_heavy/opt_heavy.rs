fn main() {
    let mut sum: i64 = 0;
    for i in 0..100000000 {
        let a: i64 = 1;
        let b: i64 = 0;
        let c: i64 = 1;
        let d: i64 = 0;
        let val = ((i * a + b) * c) + d;
        if 1 == 0 {
            sum = sum - 9999;
        } else {
            if 0 == 1 {
                sum = sum - 1111;
            } else {
                sum = sum + val;
            }
        }
    }
    println!("{}", sum);
}
