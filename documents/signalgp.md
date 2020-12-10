# SignalGP representation

Here, we give more details on how we configured SignalGP for this work, including the particular instruction sets used in each experiment.
For a broad overview of SignalGP, see [@lalejini_evolving_2018].
For exact configurations used in each experiment, see respective [experiment directories](https://github.com/amlalejini/Tag-based-Genetic-Regulation-for-LinearGP/tree/master/experiments).

## Memory model

SignalGP programs have four types of memory buffers with which to carry out computations:

- Working (register) memory
  - Call-local (* thread-local) memory.
  - Memory used by the majority of computation instructions.
- Input memory
  - Call-local (& thread-local) memory.
  - Read-only.
  - This memory is used to specify function call arguments. When a function is called on a thread (i.e.,
    a call instruction is executed), the caller's working memory is copied into the input memory of
    the new call-state, which is created on top of the thread's call stack.
  - Programs must execute explicit instructions to read from the input memory buffer (into the working
    memory buffer).
- Output memory
  - Call-local (& thread-local) memory.
  - Write-only.
  - This memory is used to specify the return values of a function call.
  - When a function returns to the previous call-state (i.e., the one just below it on the thread's
    call stack), positions that were set in the output buffer are returned to the caller's working memory
    buffer.
  - Programs must execute explicit instructions to write to the output memory buffer (from the working
    memory buffer).
- Global memory
  - This memory buffer is shared by all executing threads. Threads must use explicit instructions to access it.

Memory buffers are implemented as integer to double maps. Instruction use their integer arguments to specify locations in memory.
In this work, evolving programs do not have de-referencing functionality (i.e., memory pointers).

## Mutation operators

- Single-instruction insertions
  - Applied per instruction
- Single-instruction deletions
  - Applied per instruction
- Single-instruction substitutions
  - Applied per instruction
- Single-argument substitutions
  - Applied per argument
- Slip mutations [@lalejini_gene_2017]
  - Applied at a per-function rate.
  - Pick two random positions in function's instructions sequence: A and B
  - If A < B: duplicate sequence from A to B
  - If A > B: delete sequence from A to B
- Single-function duplications
  - Applied per-function
- Single-function deletions
  - Applied per-function
- Tag bit flips
  - Applied at a per-bit rate
  - (applies to both instruction- and function-tags)

## Instruction set

Abbreviations:

- EOP: End of program
- Reg: local register
  - Reg[0] indicates the value at the register specified by an instruction's first _argument_ (either tag-based or numeric), Reg[1] indicates the value at the register specified by an instruction's second argument, and Reg[2] indicates the value at the register specified by the instruction's third argument.
  - Reg[0], Reg[1], _etc_: Register 0, Register 1, _etc._
- Input: input buffer
  - Follows same scheme as Reg
- Output: output buffer
  - Follows same scheme as Reg
- Global: global memory buffer
  - Follows same scheme as Reg
- Arg: Instruction argument
  - Arg[i] indicates the i'th instruction argument (an integer encoded in the genome)
  - E.g., Arg[0] is an instruction's first argument

Instructions that would produce undefined behavior (e.g., division by zero) are treated as no operations.

### Default Instructions

I.e., instructions used across all diagnostic tasks.

| Instruction | Arguments Used | Description |
| :--- | :---: | :--- |
| `Nop` | 0 | No operation |
| `Not` | 1 | Reg[0] = !Reg[0] |
| `Inc` | 1 | Reg[0] = Reg[0] + 1 |
| `Dec` | 1 | Reg[0] = Reg[0] - 1 |
| `Add` | 3 | Reg[2] = Reg[0] + Reg[1] |
| `Sub` | 3 | Reg[2] = Reg[0] - Reg[1] |
| `Mult`  | 3 | Reg[2] = Reg[0] * Reg[1] |
| `Div` | 3 | Reg[2] = Reg[0] / Reg[1] |
| `Mod` | 3 | Reg[2] = Reg[0] % Reg[1] |
| `Nand` | 2 | Reg[2] = !(Reg[0] & Reg[1]) |
| `TestEqu`  | 3 | Reg[2] = Reg[0] == Reg[1] |
| `TestNEqu` | 3 | Reg[2] = Reg[0] != Reg[1] |
| `TestLess` | 3 | Reg[2] = Reg[0] < Reg[1] |
| `TestLessEqu` | 3 | Reg[2] = Reg[0] <= Reg[1] |
| `TestGreater`  | 3 | Reg[2] = Reg[0] > Reg[1] |
| `TestGreaterEqu`  | 3 | Reg[2] = Reg[0] >= Reg[1] |
| `SetMem` | 2 | Reg[0] = Arg[1] |
| `Terminal`  | 1 | Reg[0] = double value encoded by instruction tag |
| `CopyMem` | 2 | Reg[0] = Reg[1] |
| `SwapMem` | 2   | Swap(Reg[0], Reg[1]) |
| `InputToWorking`  | 2    | Reg[1] = Input[0] |
| `WorkingToOutput`  | 2    | Output[1] = Reg[0] |
| `If`      | 1   | If Reg[0] != 0, proceed. Otherwise skip to the next `Close` or EOP. |
| `While` | 1     | While Reg[0] != 0, loop. Otherwise skip to next `Close` or EOP. |
| `Close` | 0     | Indicate the end of a control block of code (e.g., loop, if). |
| `Break` | 0     | Break out of current control flow (e.g., loop). |
| `Terminate`  | 0 | Kill thread that this instruction is executing on. |
| `Fork`  | 0    | Generate an internal signal (using this instruction's tag) that can trigger a function to run in parallel. |
| `Call`  | 0    | Call a function, using this instruction's tag to determine which function is called. |
| `Routine`  | 0  | Same as call, but local memory is shared. Sort of like a jump that will jump back when the routine ends. |
| `Return`  | 0    | Return from the current function call. |

Note that `Nand` performs a bitwise operation.

### Global memory access instructions

For experimental conditions without global memory access, these instructions are replaced with no-operation
such that the instruction set remains a constant size regardless of experimental condition.

| Instruction | Arguments Used | Description |
| :--- | :---: | :--- |
| `WorkingToGlobal` | 2 | Global[1] = Reg[0] |
| `GlobalToWorking` | 2 | Reg[1] = Global[0] |
| `FullGlobalToWorking` | 0 | Copy entire global memory buffer into working memory buffer |
| `FullWorkingToGlobal` | 0 | Copy entire working memory buffer into global memory buffer |

Note that full buffer copies only copy registers that have been set (they do not necessarily stomp all over the entire destination buffer).

### Regulation instructions

For experimental conditions without regulation, these instructions are replaced with no-operation
such that the instruction set remains a constant size regardless of experimental condition.

Note that several regulation instructions have a baseline and (-) version.
The (-) versions are identical to the baseline version, except that they multiply the value they are
regulating with by -1. This eliminates any bias toward either up-/down-regulation.

Also note that the emp::MatchBin (in the Empirical library) data structure that manages function regulation
is defined in terms of tag DISTANCE, not similarity. So, decreasing function regulation values decreases
the distance between potential referring tags, and thus, unintuitively, _up-regulates_ the function.

All tag-based referencing used by regulation instructions use unregulated, raw match scores. Thus, programs
can still up-regulate a function that was previously 'turned off' with down-regulation.

| Instruction | Arguments Used | Description |
| :--- | :---: | :--- |
| `SetRegulator` | 1 | Set regulation value of function (targeted with instruction tag) to Reg[0]. |
| `SetRegulator-` | 1 | Set regulation value of function (targeted with instruction tag) to -1 * Reg[0]. |
| `SetOwnRegulator` | 1 | Set regulation value of function (currently executing) to Reg[0].|
| `SetOwnRegulator-` | 1 | Set regulation value of function (currently executing) to -1 * Reg[0]. |
| `AdjRegulator` | 1 | Regulation value of function (targeted with instruction tag) += Reg[0] |
| `AdjRegulator-` | 1 | Regulation value of function (targeted with instruction tag) -= Reg[0] |
| `AdjOwnRegulator` | 1 | Regulation value of function (currently executing) += Reg[0] |
| `AdjOwnRegulator-` | 1 | Regulation value of function (currently executing) -= Reg[0] |
| `ClearRegulator` | 0 | Clear function regulation (reset to neutral) of function targeted by instruction's tag. |
| `ClearOwnRegulator` | 0 | Clear function regulation (reset to neutral) of currently executing function |
| `SenseRegulator` | 1 | Reg[0] = regulator state of function targeted by instruction tag |
| `SenseOwnRegulator` | 1 | Reg[0] = regulator state of current function |
| `IncRegulator` | 0 | Increment regulator state of function targeted with this instruction's tag |
| `IncOwnRegulator` | 0 | Increment regulator state of currently executing function |
| `DecRegulator` | 0 | Decrement regulator state of function targeted with this instruction's tag |
| `DecOwnRegulator` | 0 | Decrement regulator state of the currently executing function |

### Task-specific instructions

Each task as a number of response instructions added to the instruction set equal to the possible set
of responses that can be expressed by a program. Each of these response instructions set a
flag on the virtual hardware indicating which response the program expressed and reset all executing
threads such that only function regulation and global memory contents persist.