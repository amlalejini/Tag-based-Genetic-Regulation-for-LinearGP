---
title: Hand-coded SignalGP example programs
---

# Handcoded SignalGP example programs

<!-- TOC -->

- [Repeated Signal Task](#repeated-signal-task)
  - [Two-signal Task - memory-based solution](#two-signal-task---memory-based-solution)
  - [Four-signal Task - memory-based solution](#four-signal-task---memory-based-solution)
  - [Eight-signal Task - memory-based solution](#eight-signal-task---memory-based-solution)
  - [Sixteen-signal Task - memory-based solution](#sixteen-signal-task---memory-based-solution)

<!-- /TOC -->

Handcoded SignalGP programs, useful for task/intuition validation.

## Repeated Signal Task

To validate that memory-based solutions are possible in our SignalGP representation, we handcoded memory-based
solutions for each of two-signal, four-signal, eight-signal, and sixteen-signal repeated signal environments.

These solutions are most not the most efficient implementations.

### Two-signal Task - memory-based solution

Pseudocode:
```
If(Global[0]%2 == 0) {
  Response-0
} else {
  Response-1
}
Global[0]+=1
```

SignalGP instructions:

```
SetMem(2,2,0)
GlobalToWorking(0,0,0)
Mod(0,2,1)
Inc(0,0,0)
WorkingToGlobal(0,0,0)
If(1,0,0)
  Response-1
Close
Response-0
```

As C++ (can be used in AltSigWorld::InitPop_Hardcoded):

```
program.PushInst(*inst_lib, "SetMem", {2, 2, 0}, {tag_t()});
program.PushInst(*inst_lib, "GlobalToWorking", {0, 0, 0}, {tag_t()});
program.PushInst(*inst_lib, "Mod", {0, 2, 1}, {tag_t()});
program.PushInst(*inst_lib, "Inc", {0, 0, 0}, {tag_t()});
program.PushInst(*inst_lib, "WorkingToGlobal", {0, 0, 0}, {tag_t()});
program.PushInst(*inst_lib, "If", {1, 0, 0}, {tag_t()});
program.PushInst(*inst_lib, "Response-1", {0, 0, 0}, {tag_t()});
program.PushInst(*inst_lib, "Close", {0, 0, 0}, {tag_t()});
program.PushInst(*inst_lib, "Response-0", {0, 0, 0}, {tag_t()});
```

### Four-signal Task - memory-based solution

SignalGP instructions:

```
// Register[0]: current environment id; Register[1]: environment id constant; Register[2]: Register[0] == Register[1]

// Get global memory value, increment environment tracker, push to global memory
GlobalToWorking(0,0) // (global memory is initialized to 0 if never set)
CopyMem(0, 3)
Inc(3)
WorkingToGlobal(3,0)

// test for response 0
SetMem(1,0)
TestEqu(0,1,2)
If(2)
  Response-0
Close

// test for response 1
SetMem(1,1)
TestEqu(0,1,2)
If(2)
  Response-1
Close

// test for response 2
SetMem(1,2)
TestEqu(0,1,2)
If(2)
  Response-2
Close

// test for response 3
SetMem(1,3)
TestEqu(0,1,2)
If(2)
  Response-3
Close
```

As C++ (can be used in AltSigWorld::InitPop_Hardcoded):

```
program.PushInst(*inst_lib, "GlobalToWorking", {0,0,0}, {tag_t()} );
program.PushInst(*inst_lib, "CopyMem",         {0,3,0}, {tag_t()} );
program.PushInst(*inst_lib, "Inc",             {3,0,0}, {tag_t()} );
program.PushInst(*inst_lib, "WorkingToGlobal", {3,0,0}, {tag_t()} );

program.PushInst(*inst_lib, "SetMem",          {1,0,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",         {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",              {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-0",      {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",           {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "SetMem",          {1,1,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",         {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",              {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-1",      {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",           {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "SetMem",            {1,2,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",           {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",                {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-2",        {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",             {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "SetMem",            {1,3,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",           {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",                {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-3",        {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",             {0,0,0}, {tag_t()});
```

### Eight-signal Task - memory-based solution

SignalGP instructions:

```
GlobalToWorking(0,0,0)
CopyMem(0,3,0)
Inc(3,0,0)
WorkingToGlobal(3,0,0)

SetMem(1,0,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-0(0,0,0)
Close(0,0,0)

SetMem(1,1,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-1(0,0,0)
Close(0,0,0)

SetMem(1,2,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-2(0,0,0)
Close(0,0,0)

SetMem(1,3,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-3(0,0,0)
Close(0,0,0)

SetMem(1,4,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-4(0,0,0)
Close(0,0,0)

SetMem(1,5,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-5(0,0,0)
Close(0,0,0)

SetMem(1,6,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-6(0,0,0)
Close(0,0,0)

SetMem(1,7,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-7(0,0,0)
Close(0,0,0)
```

As C++ (can be used in AltSigWorld::InitPop_Hardcoded):

```
program.PushInst(*inst_lib, "GlobalToWorking", {0,0,0}, {tag_t()} );
program.PushInst(*inst_lib, "CopyMem",         {0,3,0}, {tag_t()} );
program.PushInst(*inst_lib, "Inc",             {3,0,0}, {tag_t()} );
program.PushInst(*inst_lib, "WorkingToGlobal", {3,0,0}, {tag_t()} );

program.PushInst(*inst_lib, "SetMem",          {1,0,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",         {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",              {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-0",      {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",           {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "SetMem",          {1,1,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",         {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",              {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-1",      {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",           {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "SetMem",            {1,2,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",           {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",                {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-2",        {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",             {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "SetMem",            {1,3,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",           {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",                {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-3",        {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",             {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "SetMem",            {1,4,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",           {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",                {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-4",        {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",             {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "SetMem",            {1,5,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",           {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",                {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-5",        {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",             {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "SetMem",            {1,6,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",           {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",                {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-6",        {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",             {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "SetMem",            {1,7,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",           {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",                {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-7",        {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",             {0,0,0}, {tag_t()});
```


### Sixteen-signal Task - memory-based solution

Note that we limited functions to a maximum of 64 instructions for our final set of runs. The solution
below is greater than 64 instructions. It, however, can easily be split into two functions, requiring
the first to call the second. I'm not writing it split between two functions because the environment
signal tag is generated randomly during setup, so I would not be able to guarantee that the correction
function would be triggered by the environment.

SignalGP instructions:

```
GlobalToWorking(0,0,0)
CopyMem(0,3,0)
Inc(3,0,0)
WorkingToGlobal(3,0,0)

SetMem(1,0,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-0(0,0,0)
Close(0,0,0)

SetMem(1,1,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-1(0,0,0)
Close(0,0,0)

SetMem(1,2,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-2(0,0,0)
Close(0,0,0)

SetMem(1,3,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-3(0,0,0)
Close(0,0,0)

SetMem(1,4,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-4(0,0,0)
Close(0,0,0)

SetMem(1,5,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-5(0,0,0)
Close(0,0,0)

SetMem(1,6,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-6(0,0,0)
Close(0,0,0)

SetMem(1,7,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-7(0,0,0)
Close(0,0,0)

Inc(1,0,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-8(0,0,0)
Close(0,0,0)

Inc(1,0,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-9(0,0,0)
Close(0,0,0)

Inc(1,0,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-10(0,0,0)
Close(0,0,0)

Inc(1,0,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-11(0,0,0)
Close(0,0,0)

Inc(1,0,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-12(0,0,0)
Close(0,0,0)

Inc(1,0,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-13(0,0,0)
Close(0,0,0)

Inc(1,0,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-14(0,0,0)
Close(0,0,0)

Inc(1,0,0)
TestEqu(0,1,2)
If(2,0,0)
  Response-15(0,0,0)
Close(0,0,0)
```

As C++ (can be used in AltSigWorld::InitPop_Hardcoded):

```
program.PushInst(*inst_lib, "GlobalToWorking", {0,0,0}, {tag_t()} );
program.PushInst(*inst_lib, "CopyMem",         {0,3,0}, {tag_t()} );
program.PushInst(*inst_lib, "Inc",             {3,0,0}, {tag_t()} );
program.PushInst(*inst_lib, "WorkingToGlobal", {3,0,0}, {tag_t()} );

program.PushInst(*inst_lib, "SetMem",          {1,0,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",         {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",              {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-0",      {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",           {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "SetMem",          {1,1,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",         {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",              {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-1",      {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",           {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "SetMem",            {1,2,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",           {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",                {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-2",        {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",             {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "SetMem",            {1,3,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",           {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",                {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-3",        {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",             {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "SetMem",            {1,4,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",           {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",                {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-4",        {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",             {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "SetMem",            {1,5,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",           {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",                {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-5",        {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",             {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "SetMem",            {1,6,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",           {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",                {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-6",        {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",             {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "SetMem",            {1,7,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",           {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",                {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-7",        {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",             {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "SetMem",          {1,8,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",         {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",              {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-8",      {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",           {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "SetMem",          {1,8,0}, {tag_t()});
program.PushInst(*inst_lib, "Inc",             {1,0,0}, {tag_t()}); // Make a 9 to compare to
program.PushInst(*inst_lib, "TestEqu",         {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",              {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-9",      {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",           {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "Inc",              {1,0,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",          {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",               {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-10",      {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",            {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "Inc",               {1,0,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",           {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",                {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-11",       {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",             {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "Inc",               {1,0,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",           {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",                {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-12",       {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",             {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "Inc",               {1,0,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",           {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",                {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-13",       {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",             {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "Inc",               {1,0,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",           {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",                {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-14",       {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",             {0,0,0}, {tag_t()});

program.PushInst(*inst_lib, "Inc",               {1,0,0}, {tag_t()});
program.PushInst(*inst_lib, "TestEqu",           {0,1,2}, {tag_t()});
program.PushInst(*inst_lib, "If",                {2,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Response-15",       {0,0,0}, {tag_t()});
program.PushInst(*inst_lib, "Close",             {0,0,0}, {tag_t()});
```