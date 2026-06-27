fn main() {
    let n: i64 = 1000000;
    let mut total: i64 = 0;

    for i in 1..=n {
        let mut x = i;
        while x != 1 {
            if x % 2 == 0 {
                x = x / 2;
            } else {
                x = 3 * x + 1;
            }
            total += 1;
        }
    }

    println!("{}", total);
}
