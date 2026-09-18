/**
 * x86-64 Simulator in JavaScript
 * Supports a subset of instructions emitted by our compiler.
 */

export class X86Simulator {
    constructor() {
        // Memory and registers
        this.memory = new ArrayBuffer(1024 * 1024); // 1 MB
        this.view = new DataView(this.memory);
        this.bytes = new Uint8Array(this.memory);
        
        // 64-bit registers
        // For simplicity, we use Number (53-bit int precision) and Float64.
        this.registers = {
            rax: 0, rcx: 0, rdx: 0, rbx: 0, rsp: 1024 * 1024 - 8, rbp: 1024 * 1024 - 8,
            rsi: 0, rdi: 0, r8: 0, r9: 0, r10: 0, r11: 0, r12: 0, r13: 0, r14: 0, r15: 0
        };
        // XMM registers for floats
        this.xmm = {
            xmm0: 0.0, xmm1: 0.0, xmm2: 0.0, xmm3: 0.0,
            xmm4: 0.0, xmm5: 0.0, xmm6: 0.0, xmm7: 0.0
        };

        this.flags = { ZF: false, SF: false, CF: false, OF: false };
        this.heapPtr = 100 * 1024;
        
        this.output = "";
        this.ip = 0; // Instruction Pointer (index in instructions array)
        this.labels = {};
        this.instructions = [];
        this.rodata = {}; // string constants
    }

    reset() {
        this.registers.rsp = this.memory.byteLength - 8;
        this.registers.rbp = this.memory.byteLength - 8;
        this.registers.rax = 0;
        this.output = '';
        this.ip = 0;
        this.flags = { ZF: false, SF: false, CF: false, OF: false };
    }

    readString(addr) {
        if (typeof addr === 'string') return addr; // fallback
        let str = '';
        let i = 0;
        while (true) {
            let c = this.view.getUint8(addr + i);
            if (c === 0 || i > 10000) break;
            str += String.fromCharCode(c);
            i++;
        }
        return str;
    }

    parse(asm) {
        this.labels = {};
        this.instructions = [];
        this.rodata = {};
        
        const lines = asm.split('\n');
        let inData = false;
        let inRodata = false;
        let inText = false;
        
        for (let i = 0; i < lines.length; i++) {
            let line = lines[i].trim();
            if (!line || line.startsWith('#')) continue;
            
            if (line === '.data') { inData = true; inRodata = false; inText = false; continue; }
            if (line === '.section .rodata' || line === '.rodata') { inData = false; inRodata = true; inText = false; continue; }
            if (line === '.text') { inData = false; inRodata = false; inText = true; continue; }
            if (line.startsWith('.globl') || line.startsWith('.section')) continue;

            // Parse labels (standalone or inline)
            let labelMatch = line.match(/^([a-zA-Z0-9_\.]+):/);
            if (labelMatch) {
                const label = labelMatch[1];
                if (inText) {
                    this.labels[label] = this.instructions.length;
                } else if (inRodata || inData) {
                    if (line.includes('.string')) {
                        let str = line.substring(line.indexOf('"') + 1, line.lastIndexOf('"'));
                        str = str.replace(/\\n/g, '\n').replace(/\\t/g, '\t');
                        const addr = this.heapPtr;
                        this.heapPtr += str.length + 1;
                        for (let j = 0; j < str.length; j++) {
                            this.view.setUint8(addr + j, str.charCodeAt(j));
                        }
                        this.view.setUint8(addr + str.length, 0);
                        this.rodata[label] = { type: 'string', value: str, addr: addr };
                        this.labels[label] = addr;
                    } else if (line.includes('.double')) {
                        let num = parseFloat(line.split('.double')[1].trim());
                        this.rodata[label] = { type: 'double', value: num };
                    } else {
                        // Expect on next line
                        let valLine = lines[i+1]?.trim() || "";
                        if (valLine.startsWith('.string')) {
                            let str = valLine.substring(valLine.indexOf('"') + 1, valLine.lastIndexOf('"'));
                            str = str.replace(/\\n/g, '\n').replace(/\\t/g, '\t');
                            this.rodata[label] = { type: 'string', value: str };
                            i++;
                        } else if (valLine.startsWith('.double')) {
                            let num = parseFloat(valLine.split(' ')[1]);
                            this.rodata[label] = { type: 'double', value: num };
                            i++;
                        }
                    }
                }
                
                // Remove label from line to parse potential inline instruction
                line = line.substring(labelMatch[0].length).trim();
                if (!line) continue;
            }

            if (inText) {
                // Parse instruction format: opcode arg1, arg2
                const firstSpace = line.indexOf(' ');
                if (firstSpace === -1) {
                    this.instructions.push({ op: line, args: [] });
                } else {
                    const op = line.substring(0, firstSpace);
                    const argsStr = line.substring(firstSpace + 1).trim();
                    const args = argsStr.split(',').map(a => a.trim());
                    this.instructions.push({ op, args });
                }
            }
        }
    }

    getReg(regStr) {
        regStr = regStr.replace('%', '');
        if (regStr === 'eax' || regStr === 'al') return 'rax';
        if (regStr === 'ebx' || regStr === 'bl') return 'rbx';
        if (regStr === 'ecx' || regStr === 'cl') return 'rcx';
        if (regStr === 'edx' || regStr === 'dl') return 'rdx';
        return regStr;
    }

    readMemFloat(addr) { return this.view.getFloat64(addr, true); }
    readMemInt(addr) { return Number(this.view.getBigInt64(addr, true)); }
    readMemByte(addr) { return this.view.getUint8(addr); }

    writeMemFloat(addr, val) { this.view.setFloat64(addr, val, true); }
    writeMemInt(addr, val) { this.view.setBigInt64(addr, BigInt(Math.floor(val)), true); }
    writeMemByte(addr, val) { this.view.setUint8(addr, val); }

    readVal(arg) {
        if (arg.startsWith('$')) {
            return Number(arg.substring(1));
        } else if (arg.startsWith('%xmm')) {
            return this.xmm[arg.substring(1)];
        } else if (arg.startsWith('%')) {
            return this.registers[this.getReg(arg)];
        } else if (arg.includes('(%')) {
            // Memory read: offset(%reg) or (%reg)
            let offset = 0;
            let regMatch = arg.match(/\((%.+)\)/);
            if (regMatch) {
                const regStr = regMatch[1];
                let prefix = arg.substring(0, arg.indexOf('('));
                if (prefix) offset = Number(prefix);
                const addr = this.registers[this.getReg(regStr)] + offset;
                // Simplified: assuming memory reads are 8 bytes
                return this.readMemInt(addr); 
            }
        } else if (arg.includes('(%rip)')) {
            // rip relative, it's a label in rodata
            const label = arg.split('(')[0];
            return label; // return label name
        }
        return 0;
    }

    writeVal(arg, val, isFloat = false, isByte = false) {
        if (arg.startsWith('%xmm')) {
            this.xmm[arg.substring(1)] = val;
        } else if (arg.startsWith('%')) {
            this.registers[this.getReg(arg)] = val;
        } else if (arg.includes('(%')) {
            let offset = 0;
            let regMatch = arg.match(/\((%.+)\)/);
            if (regMatch) {
                const regStr = regMatch[1];
                let prefix = arg.substring(0, arg.indexOf('('));
                if (prefix) offset = Number(prefix);
                const addr = this.registers[this.getReg(regStr)] + offset;
                if (isFloat) this.writeMemFloat(addr, val);
                else if (isByte) this.writeMemByte(addr, val);
                else this.writeMemInt(addr, val);
            }
        }
    }

    run() {
        this.reset();
        if (this.labels['main'] === undefined) {
            this.output += "Error: No main function found\n";
            return this.output;
        }
        this.ip = this.labels['main'];
        const callStack = [];

        let limit = 100000; // prevent infinite loops
        while (this.ip < this.instructions.length && limit > 0) {
            limit--;
            const inst = this.instructions[this.ip];
            const op = inst.op;
            const args = inst.args;
            this.ip++;

            try {
                // console.log(`IP: ${this.ip}, OP: ${op} ${args.join(',')} | RAX=${this.registers.rax} RSI=${this.registers.rsi}`);
                switch (op) {
                    case 'pushq': {
                        let val = this.readVal(args[0]);
                        this.registers.rsp -= 8;
                        this.writeMemInt(this.registers.rsp, val);
                        break;
                    }
                    case 'popq': {
                        let val = this.readMemInt(this.registers.rsp);
                        this.registers.rsp += 8;
                        this.writeVal(args[0], val);
                        break;
                    }
                    case 'movq': {
                        let val = this.readVal(args[0]);
                        if (typeof val === 'string' && this.rodata[val]) {
                            this.writeVal(args[1], val); 
                        } else if (typeof val === 'string') {
                             this.writeVal(args[1], val);
                        } else if (args[0].includes('(%')) {
                             this.writeVal(args[1], this.readMemInt(this.registers[this.getReg(args[0].match(/\((%.+)\)/)[1])] + Number(args[0].split('(')[0]||0)));
                        } else {
                             this.writeVal(args[1], val);
                        }
                        break;
                    }
                    case 'movsd': {
                        if (args[0].includes('(%rip)')) {
                            let label = args[0].split('(')[0];
                            this.writeVal(args[1], this.rodata[label].value, true);
                        } else if (args[0].includes('(%')) {
                             let offset = Number(args[0].split('(')[0]||0);
                             let reg = this.getReg(args[0].match(/\((%.+)\)/)[1]);
                             this.writeVal(args[1], this.readMemFloat(this.registers[reg] + offset), true);
                        } else if (args[1].includes('(%')) {
                             let val = this.xmm[args[0].substring(1)];
                             let offset = Number(args[1].split('(')[0]||0);
                             let reg = this.getReg(args[1].match(/\((%.+)\)/)[1]);
                             this.writeMemFloat(this.registers[reg] + offset, val);
                        } else {
                             let val = this.xmm[args[0].substring(1)];
                             this.xmm[args[1].substring(1)] = val;
                        }
                        break;
                    }
                    case 'movb': {
                         this.writeVal(args[1], this.readVal(args[0]), false, true);
                         break;
                    }
                    case 'movzbq': {
                        let val;
                        if (args[0].includes('(%')) {
                            let offset = Number(args[0].split('(')[0]||0);
                            let reg = this.getReg(args[0].match(/\((%.+)\)/)[1]);
                            val = this.view.getUint8(this.registers[reg] + offset);
                        } else {
                            val = this.readVal(args[0]) & 0xFF; // zero extend byte
                        }
                        this.writeVal(args[1], val);
                        break;
                    }
                    case 'leaq': {
                        if (args[0].includes('(%rip)')) {
                            let label = args[0].split('(')[0];
                            if (this.labels[label] !== undefined) {
                                this.writeVal(args[1], this.labels[label]);
                            } else {
                                this.writeVal(args[1], label);
                            }
                        } else {
                            let offset = Number(args[0].split('(')[0]||0);
                            let reg = this.getReg(args[0].match(/\((%.+)\)/)[1]);
                            this.writeVal(args[1], this.registers[reg] + offset);
                        }
                        break;
                    }
                    case 'addq': {
                        let v1 = this.readVal(args[0]);
                        let v2 = this.readVal(args[1]);
                        this.writeVal(args[1], v2 + v1);
                        break;
                    }
                    case 'subq': {
                        let v1 = this.readVal(args[0]);
                        let v2 = this.readVal(args[1]);
                        this.writeVal(args[1], v2 - v1);
                        break;
                    }
                    case 'imulq': {
                        if (args.length === 1) {
                            this.registers.rax = this.registers.rax * this.readVal(args[0]);
                        } else {
                            this.writeVal(args[1], this.readVal(args[1]) * this.readVal(args[0]));
                        }
                        break;
                    }
                    case 'cqto': {
                        this.registers.rdx = this.registers.rax < 0 ? -1 : 0;
                        break;
                    }
                    case 'idivq': {
                        let divisor = this.readVal(args[0]);
                        if (divisor === 0) throw new Error("Division by zero");
                        // In 64-bit simulator we ignore rdx for dividend since JS handles safe large ints natively up to 2^53
                        let dividend = this.registers.rax;
                        let quotient = Math.trunc(dividend / divisor);
                        let remainder = dividend % divisor;
                        this.registers.rax = quotient;
                        this.registers.rdx = remainder;
                        break;
                    }
                    case 'negq': {
                        this.writeVal(args[0], -this.readVal(args[0]));
                        break;
                    }
                    case 'xorpd': {
                        this.xmm[args[1].substring(1)] = 0.0;
                        break;
                    }
                    case 'addsd': {
                        let v = this.readVal(args[0]);
                        this.xmm[args[1].substring(1)] += v;
                        break;
                    }
                    case 'subsd': {
                        let v = this.readVal(args[0]);
                        this.xmm[args[1].substring(1)] -= v;
                        break;
                    }
                    case 'mulsd': {
                        let v = this.readVal(args[0]);
                        this.xmm[args[1].substring(1)] *= v;
                        break;
                    }
                    case 'divsd': {
                        let v = this.readVal(args[0]);
                        this.xmm[args[1].substring(1)] /= v;
                        break;
                    }
                    case 'cvtsi2sdq': {
                        let v = this.readVal(args[0]);
                        this.xmm[args[1].substring(1)] = v;
                        break;
                    }
                    case 'cvttsd2siq': {
                        let v = this.readVal(args[0]);
                        this.writeVal(args[1], Math.trunc(v));
                        break;
                    }
                    case 'cmpq': {
                        let src = this.readVal(args[0]);
                        let dst = this.readVal(args[1]);
                        this.flags.ZF = (dst === src);
                        this.flags.SF = ((dst - src) < 0);
                        break;
                    }
                    case 'ucomisd': {
                        let src = this.readVal(args[0]);
                        let dst = this.readVal(args[1]);
                        this.flags.ZF = (dst === src);
                        this.flags.CF = (dst < src);
                        break;
                    }
                    case 'jmp': {
                        this.ip = this.labels[args[0]];
                        break;
                    }
                    case 'je': {
                        if (this.flags.ZF) this.ip = this.labels[args[0]];
                        break;
                    }
                    case 'jne': {
                        if (!this.flags.ZF) this.ip = this.labels[args[0]];
                        break;
                    }
                    case 'jl': {
                        if (this.flags.SF !== this.flags.OF) this.ip = this.labels[args[0]];
                        break;
                    }
                    case 'jle': {
                        if (this.flags.ZF || this.flags.SF !== this.flags.OF) this.ip = this.labels[args[0]];
                        break;
                    }
                    case 'setl': { this.writeVal(args[0], (this.flags.SF !== this.flags.OF) ? 1 : 0); break; }
                    case 'setle': { this.writeVal(args[0], (this.flags.ZF || this.flags.SF !== this.flags.OF) ? 1 : 0); break; }
                    case 'setg': { this.writeVal(args[0], (!this.flags.ZF && this.flags.SF === this.flags.OF) ? 1 : 0); break; }
                    case 'setge': { this.writeVal(args[0], (this.flags.SF === this.flags.OF) ? 1 : 0); break; }
                    case 'sete': { this.writeVal(args[0], this.flags.ZF ? 1 : 0); break; }
                    case 'setne': { this.writeVal(args[0], !this.flags.ZF ? 1 : 0); break; }
                    case 'setb': { this.writeVal(args[0], this.flags.CF ? 1 : 0); break; } 
                    case 'setbe': { this.writeVal(args[0], (this.flags.CF || this.flags.ZF) ? 1 : 0); break; }
                    case 'seta': { this.writeVal(args[0], (!this.flags.CF && !this.flags.ZF) ? 1 : 0); break; }
                    case 'setae': { this.writeVal(args[0], !this.flags.CF ? 1 : 0); break; } 
                    case 'movl': {
                         this.writeVal(args[1], this.readVal(args[0]));
                         break;
                    }
                    case 'call': {
                        const target = args[0];
                        if (target === 'printf@PLT') {
                            let fmtAddr = this.registers.rdi;
                            let fmt = this.readString(fmtAddr);
                            if (fmt.includes('%ld') || fmt.includes('%d') || fmt.includes('%c')) {
                                let val = this.registers.rsi;
                                if (fmt.includes('%c')) {
                                    this.output += String.fromCharCode(val);
                                } else {
                                    this.output += val.toString();
                                }
                            } else if (fmt.includes('%lf')) {
                                this.output += this.xmm.xmm0.toFixed(6);
                            } else if (fmt.includes('%s')) {
                                let strAddr = this.registers.rsi;
                                this.output += this.readString(strAddr);
                            } else {
                                this.output += fmt;
                            }
                        } else if (target === 'malloc@PLT' || target === 'calloc@PLT') {
                            let size = (target === 'calloc@PLT') ? (this.registers.rdi * this.registers.rsi) : this.registers.rdi;
                            this.registers.rax = this.heapPtr;
                            this.heapPtr += size;
                            if (this.heapPtr % 8 !== 0) this.heapPtr += 8 - (this.heapPtr % 8);
                        } else if (target === 'free@PLT') {
                            // ignore free in simulator
                        } else {
                            callStack.push(this.ip);
                            this.ip = this.labels[target];
                        }
                        break;
                    }
                    case 'ret': {
                        if (callStack.length === 0) {
                            return this.output;
                        }
                        this.ip = callStack.pop();
                        break;
                    }
                    case 'leave': {
                        this.registers.rsp = this.registers.rbp;
                        this.registers.rbp = this.readMemInt(this.registers.rsp);
                        this.registers.rsp += 8;
                        break;
                    }
                }
            } catch (err) {
                this.output += `\n[Simulator Error at instruction ${this.ip - 1} (${op} ${args.join(',')}): ${err.message}]`;
                return this.output;
            }
        }
        
        if (limit <= 0) this.output += "\n[Simulation Terminated: Loop limit reached]";
        return this.output;
    }
}
