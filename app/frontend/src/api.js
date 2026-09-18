import { X86Simulator } from './simulator.js';

let compilerModule = null;
let compileC = null;

async function initCompiler() {
  if (compileC) return;
  if (!window.createCompiler) {
    throw new Error("Compiler WASM module not loaded.");
  }
  compilerModule = await window.createCompiler();
  compileC = compilerModule.cwrap('compile_code', 'string', ['string', 'boolean']);
}

export async function runCode(code, optimize) {
  try {
    await initCompiler();
    const t0 = performance.now();
    
    // 1. Compile C++ to Assembly (WASM)
    const jsonResult = compileC(code, optimize);
    const compileTime = performance.now() - t0;
    
    let result = null;
    try {
      result = JSON.parse(jsonResult);
    } catch (e) {
      return { success: false, error: { type: "server", message: "Invalid JSON from compiler." } };
    }

    if (!result.success) {
      return result; // contains error
    }

    // 2. Simulate the x86-64 assembly in JS
    const simulator = new X86Simulator();
    
    const t1 = performance.now();
    simulator.parse(result.asm);
    const stdout = simulator.run();
    const execTime = performance.now() - t1;

    return {
      success: true,
      ast: result.ast,
      tokens: result.tokens || [],
      asm: result.asm,
      run: {
        stdout: stdout,
        stderr: "",
        exit_code: 0,
        timed_out: false
      },
      metrics: {
        compile_ms: Number(compileTime.toFixed(2)),
        assemble_ms: 0,
        exec_ms: Number(execTime.toFixed(2)),
        binary_size_bytes: result.asm.length
      }
    };

  } catch (error) {
    return {
      success: false,
      error: { type: "server", line: 0, col: 0, message: error.message || "Failed to compile/simulate." }
    };
  }
}
