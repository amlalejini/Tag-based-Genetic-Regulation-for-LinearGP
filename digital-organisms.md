# SignalGP Digital Organisms

Here, we give more details on the setup of the SignalGP digital organisms used in the diagnostic
experiments. For a broad overview of SignalGP, see [(Lalejini and Ofria, 2018)](https://doi.org/10.1145/3205455.3205523). For specific parameter choices, see experiment-specific configuration descriptions (TODO - link here).

For DISHTINY SignalGP details, see DISHTINY docs.

## Memory model

TODO

## Instruction Set

Abbreviations:

- EOP: End of program
- Reg: local register
  - Reg[0] indicates the value at the register specified by an instruction's first _argument_ (either tag-based or numeric), Reg[1] indicates the value at the register specified by an instruction's second argument, and Reg[2] indicates the value at the register specified by the instruction's third argument.
  - Reg[0], Reg[1], _etc_: Register 0, Register 1, _etc._
- Input: input buffer
  - Follows same scheme as Reg
- Output: output buffer
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

### Global memory access instructions

For experimental conditions without global memory access, these instructions are replaced with no-operation
such that the instruction set remains a constant size regardless of experimental condition.

| Instruction | Arguments Used | Description |
| :--- | :---: | :--- |
| `WorkingToGlobal` | 2 | Global[1] = Reg[0] |
| `GlobalToWorking` | 2 | Reg[1] = Global[0] |

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
| `SetRegulator` | 1 | |
| `SetRegulator-` | 1 | |
| `SetOwnRegulator` | 1 | |
| `SetOwnRegulator-` | 1 | |
| `AdjRegulator` | 1 | |
| `AdjRegulator-` | 1 | |
| `AdjOwnRegulator` | 1 | |
| `AdjOwnRegulator-` | 1 | |
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
of responses that can be expressed by a digital organism. Each of these response instructions set a
flag on the virtual hardware indicating which response the organism expressed and reset all executing
threads such that only function regulation and global memory contents persist.

## References

Lalejini, A., & Ofria, C. (2018). Evolving event-driven programs with SignalGP. Proceedings of the Genetic and Evolutionary Computation Conference on - GECCO ’18, 1135–1142. https://doi.org/10.1145/3205455.3205523
