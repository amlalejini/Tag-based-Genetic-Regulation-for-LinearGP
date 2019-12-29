/*
// Algorithm:
If(Global[0]%2 == 0) {
  Response-0
} else {
  Response-1
}
Global[0]+=1
*/

SetMem(2, 2)
GlobalToWorking(0,0)
Mod(0, 2, 1)
Inc(0)
WorkingToGlobal(0, 0)
If(1)
  Response-1
Close
Response-0

/* As c++:
program.PushInst(*inst_lib, "SetMem", {2, 2, 0}, {tag_t()});
program.PushInst(*inst_lib, "GlobalToWorking", {0, 0, 0}, {tag_t()});
program.PushInst(*inst_lib, "Mod", {0, 2, 1}, {tag_t()});
program.PushInst(*inst_lib, "Inc", {0, 0, 0}, {tag_t()});
program.PushInst(*inst_lib, "WorkingToGlobal", {0, 0, 0}, {tag_t()});
program.PushInst(*inst_lib, "If", {1, 0, 0}, {tag_t()});
program.PushInst(*inst_lib, "Response-1", {0, 0, 0}, {tag_t()});
program.PushInst(*inst_lib, "Close", {0, 0, 0}, {tag_t()});
program.PushInst(*inst_lib, "Response-0", {0, 0, 0}, {tag_t()});
*/
