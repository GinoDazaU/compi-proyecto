// Example programs for the editor menu. Each highlights a feature
// of the supported C++ subset (no templates or lambdas).

export const EXAMPLES = [
  {
    name: "Fibonacci",
    code: `// Iterative Fibonacci calculation
int main() {
    println("Iterative Fibonacci (first 10 numbers):");
    int a = 0;
    int b = 1;
    for (int i = 0; i < 10; i = i + 1) {
        print("F("); print(i); print(") = "); print(a); println(' ');
        int next = a + b;
        a = b;
        b = next;
    }
    return 0;
}
`,
  },
  {
    name: "Fibonacci (recursion)",
    code: `// Recursive Fibonacci calculation
int fib(int n) {
    if (n <= 1) { return n; }
    return fib(n - 1) + fib(n - 2);
}

int main() {
    println("Recursive Fibonacci (first 10 numbers):");
    for (int i = 0; i < 10; i = i + 1) {
        print("fib("); print(i); print(") = ");
        println(fib(i));
    }
    return 0;
}
`,
  },
  {
    name: "Factorial (recursion)",
    code: `// Recursive Factorial calculation
int factorial(int n) {
    if (n <= 1) { return 1; }
    return n * factorial(n - 1);
}

int main() {
    println("Recursive Factorial (1 to 10):");
    for (int i = 1; i <= 10; i = i + 1) {
        print("fact("); print(i); print(") = ");
        println(factorial(i));
    }
    return 0;
}
`,
  },
  {
    name: "FizzBuzz",
    code: `// Standard FizzBuzz up to 15
int main() {
    println("FizzBuzz output (1 to 15):");
    for (int i = 1; i <= 15; i = i + 1) {
        print(i); print(": ");
        if (i % 15 == 0) {
            println("FizzBuzz");
        } else if (i % 3 == 0) {
            println("Fizz");
        } else if (i % 5 == 0) {
            println("Buzz");
        } else {
            println("-");
        }
    }
    return 0;
}
`,
  },
  {
    name: "Bubble sort",
    code: `// Bubble sort implementation sorting an array of 6 integers
int main() {
    int arr[6] = {5, 2, 9, 1, 7, 3};
    int n = 6;
    
    print("Before sorting: ");
    for (int i = 0; i < n; i = i + 1) {
        print(arr[i]); print(' ');
    }
    println(' ');

    // Perform Bubble Sort
    for (int i = 0; i < n - 1; i = i + 1) {
        for (int j = 0; j < n - 1 - i; j = j + 1) {
            if (arr[j] > arr[j + 1]) {
                int tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }

    print("After sorting:  ");
    for (int i = 0; i < n; i = i + 1) {
        print(arr[i]); print(' ');
    }
    println(' ');
    return 0;
}
`,
  },
  {
    name: "Quicksort (pointers)",
    code: `// Quicksort sorting an array of 8 integers using pointers
void quicksort(int* a, int lo, int hi) {
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
    
    print("Before Quicksort: ");
    for (int i = 0; i < 8; i = i + 1) {
        print(arr[i]); print(' ');
    }
    println(' ');

    quicksort(arr, 0, 7);

    print("After Quicksort:  ");
    for (int i = 0; i < 8; i = i + 1) {
        print(arr[i]); print(' ');
    }
    println(' ');
    return 0;
}
`,
  },
  {
    name: "Structs",
    code: `// Structs and pointers demo
struct Point {
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
    println("Struct members values:");
    print("p.x = "); println(p.x);
    print("p.y = "); println(p.y);
    print("Sum (p.x + p.y) = ");
    println(sumXY(&p));
    return 0;
}
`,
  },
  {
    name: "Pointers & memory",
    code: `// Dynamic memory allocation and pointers
int main() {
    int n = 5;
    println("Allocating array of size 5...");
    int* arr = new int[n];
    
    // Fill array
    for (int i = 0; i < n; i = i + 1) {
        arr[i] = i * i;
    }
    
    // Print contents
    print("Array elements: ");
    for (int i = 0; i < n; i = i + 1) {
        print(arr[i]);
        print(' ');
    }
    println(' ');
    
    println("Deleting allocated array.");
    delete[] arr;
    return 0;
}
`,
  },
  {
    name: "Floats & promotion",
    code: `// Floating-point arithmetic and type promotion
int main() {
    int a = 7;
    float b = 2.0;
    float avg = (a + b) / 2.0;
    print("Average of 7 and 2.0 is: ");
    println(avg);
    
    char c = 'A';
    int code = c + 1;
    print("Char value '"); print(c); print("' promoted to int plus 1: ");
    println(code);
    return 0;
}
`,
  },
  {
    name: "2D matrix",
    code: `// 2D Array / Matrix manipulation
int main() {
    int m[3][3];
    println("Initializing 3x3 matrix...");
    for (int i = 0; i < 3; i = i + 1) {
        for (int j = 0; j < 3; j = j + 1) {
            m[i][j] = i * 3 + j;
        }
    }
    
    println("Matrix output:");
    for (int i = 0; i < 3; i = i + 1) {
        for (int j = 0; j < 3; j = j + 1) {
            print(m[i][j]);
            print(" ");
        }
        println(' ');
    }
    return 0;
}
`,
  },
  {
    name: "Type inference (auto)",
    code: `// Type inference with 'auto' keyword
int main() {
    auto greeting = "Hello, auto type inference!";
    println(greeting);
    
    auto n = 42;
    auto half = n / 2;
    print("Original number (int): "); println(n);
    print("Divided by 2 (auto): "); println(half);
    return 0;
}
`,
  },
  {
    name: "Optimization demo",
    code: `// Optimizer demonstration (runs faster with --opt)
int main() {
    println("Running heavy loop with algebraic operations & dead branches...");
    int sum = 0;
    
    // Large loop to measure performance
    for (int i = 0; i < 1000000000; i = i + 1) {
        int a = 1;
        int b = 0;
        int c = 1;
        int d = 0;
        // Simplified by optimizer to: val = i
        int val = (((i * a + b) * c) + d);
        
        // Dead branches are eliminated by Dead Code Elimination pass
        if (1 == 0) {
            sum = sum - 9999;
        } else {
            if (0 == 1) {
                sum = sum - 1111;
            } else {
                sum = sum + val;
            }
        }
    }
    print("Resulting sum: ");
    println(sum);
    return 0;
}
`,
  },
];
