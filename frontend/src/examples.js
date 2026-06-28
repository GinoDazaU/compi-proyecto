// Programas de ejemplo para el menú del editor. Cada uno resalta una feature
// del subconjunto de C++ soportado (sin templates ni lambdas).

export const EXAMPLES = [
  {
    name: "Fibonacci",
    code: `int main() {
    int a = 0;
    int b = 1;
    for (int i = 0; i < 10; i = i + 1) {
        print(a);
        print(' ');
        int next = a + b;
        a = b;
        b = next;
    }
    println(' ');
    return 0;
}
`,
  },
  {
    name: "Factorial (recursion)",
    code: `int factorial(int n) {
    if (n <= 1) { return 1; }
    return n * factorial(n - 1);
}

int main() {
    for (int i = 1; i <= 10; i = i + 1) {
        print(i);
        print("! = ");
        println(factorial(i));
    }
    return 0;
}
`,
  },
  {
    name: "FizzBuzz",
    code: `int main() {
    for (int i = 1; i <= 15; i = i + 1) {
        if (i % 15 == 0) {
            println("FizzBuzz");
        } else if (i % 3 == 0) {
            println("Fizz");
        } else if (i % 5 == 0) {
            println("Buzz");
        } else {
            println(i);
        }
    }
    return 0;
}
`,
  },
  {
    name: "Bubble sort",
    code: `int main() {
    int arr[6] = {5, 2, 9, 1, 7, 3};
    int n = 6;
    for (int i = 0; i < n - 1; i = i + 1) {
        for (int j = 0; j < n - 1 - i; j = j + 1) {
            if (arr[j] > arr[j + 1]) {
                int tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }
    for (int i = 0; i < n; i = i + 1) {
        print(arr[i]);
        print(' ');
    }
    println(' ');
    return 0;
}
`,
  },
  {
    name: "Quicksort (pointers)",
    code: `void quicksort(int* a, int lo, int hi) {
    if (lo >= hi) { return; }
    int pivot = a[hi];
    int i = lo;
    for (int j = lo; j < hi; j = j + 1) {
        if (a[j] < pivot) {
            int t = a[i]; a[i] = a[j]; a[j] = t;
            i = i + 1;
        }
    }
    int t = a[i]; a[i] = a[hi]; a[hi] = t;
    quicksort(a, lo, i - 1);
    quicksort(a, i + 1, hi);
}

int main() {
    int arr[8] = {9, 3, 7, 1, 8, 2, 6, 4};
    quicksort(arr, 0, 7);
    for (int i = 0; i < 8; i = i + 1) {
        print(arr[i]);
        print(' ');
    }
    println(' ');
    return 0;
}
`,
  },
  {
    name: "Structs",
    code: `struct Point {
    int x;
    int y;
};

int sumXY(Point* p) {
    return p->x + p->y;
}

int main() {
    Point p;
    p.x = 10;
    p.y = 25;
    print("sum = ");
    println(sumXY(&p));
    return 0;
}
`,
  },
  {
    name: "Pointers & memory",
    code: `int main() {
    int n = 5;
    int* arr = new int[n];
    for (int i = 0; i < n; i = i + 1) {
        arr[i] = i * i;
    }
    for (int i = 0; i < n; i = i + 1) {
        print(arr[i]);
        print(' ');
    }
    println(' ');
    delete[] arr;
    return 0;
}
`,
  },
  {
    name: "Floats & promotion",
    code: `int main() {
    int a = 7;
    float b = 2.0;
    float avg = (a + b) / 2.0;
    print("avg = ");
    println(avg);
    char c = 'A';
    int code = c + 1;
    print(c);
    print(" + 1 -> ");
    println(code);
    return 0;
}
`,
  },
  {
    name: "2D matrix",
    code: `int main() {
    int m[3][3];
    for (int i = 0; i < 3; i = i + 1) {
        for (int j = 0; j < 3; j = j + 1) {
            m[i][j] = i * 3 + j;
        }
    }
    for (int i = 0; i < 3; i = i + 1) {
        for (int j = 0; j < 3; j = j + 1) {
            print(m[i][j]);
            print(' ');
        }
        println(' ');
    }
    return 0;
}
`,
  },
  {
    name: "Type inference (auto)",
    code: `int main() {
    auto greeting = "Hello, world!";
    println(greeting);
    auto n = 42;
    auto half = n / 2;
    print("half = ");
    println(half);
    return 0;
}
`,
  },
];
