fn main() {
    let w: i64 = 600;
    let h: i64 = 600;
    let max_iter: i64 = 256;
    let mut total: i64 = 0;

    for py in 0..h {
        for px in 0..w {
            let x0 = px as f64 * 1.0 / w as f64 * 3.5 - 2.5;
            let y0 = py as f64 * 1.0 / h as f64 * 2.0 - 1.0;
            let mut x: f64 = 0.0;
            let mut y: f64 = 0.0;
            let mut iter: i64 = 0;
            while iter < max_iter && x * x + y * y < 4.0 {
                let xt = x * x - y * y + x0;
                y = 2.0 * x * y + y0;
                x = xt;
                iter += 1;
            }
            total += iter;
        }
    }

    println!("{}", total);
}
