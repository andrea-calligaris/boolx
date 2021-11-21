# BoolX

**BoolX** is an [esoteric programming language](https://esolangs.org) that works with **binary numbers**. The cool part is that it allows to do arithmetics and other operations with numbers of theoretically infinite length.
Every instruction in _BoolX_ is an _ASCII_ character.

I invented it in 2008.

## Compacted program example

```
_+_+_+_+_+_+^#///////@>>_+_+_+_+_+^+_]=^+^+_
+^+_+^+*]=_+_+_+_+_+^+_]=<^+_+^+^+^#$///////
@>_+_+_+_+_+^+_]=^+_+^+^+^+^+*]=_+_+_+_+_+^+
_]<<#>#$@>>&#$///////@~:&>&</:">">?>^#~!>#~;
!<;;>>?_<<?>?>^>^!>^>_;!>?>^>_!>>^;;!<<?>?>^
>_!>>^;!>?>>^!>>_;;;|+>+>>+<<<':&>&</:">">>#
/@~!<;;>>?_<<?>?>^>^!>_>_;!>?>^>_!>^>^;;!<<?
>?>>_!>>^;!>?>^>^!>>_;;;|+>+>>+<<<':&"#~;/:?
>^<;"!+';>?<!<%_#~;/:?#~!*;-':&>_>^>^+_+_+_+
^+^=>_+_+_+_+^+^=|:"-/'!$>#>#@<&<+$////////'
:>?!+"~;=;$//#>#@<&<?>>>]!>>>>];|-$/////////
'
```
Output:

`1000000 + 11101 = 1011101` (64 + 29 = 93)

Out of the box, like [_brainfuck_](https://esolangs.org/wiki/Brainfuck), _BoolX_ can only print the corresponding _ASCII_ character of a cell. Because of this, the above program actually contains a function that, calling in turn other functions, manually prints the bits of a cell on screen.

## "Hello, world!" program

```
{ hello_world.bx }
{ Prints "Hello, world!" to standard output. }

_+_+_+^+_+_+^]=  {  72 }
^+_+^+_+_+^+^]=  { 101 }
_+_+^+^+_+^+^]]= { 108 x2 }
^+^+^+^+_+^+^]=  { 111 }
_+_+^+^+_+^+_]=  {  44 }
_+_+_+_+_+^+_]=  {  32 }
^+^+^+_+^+^+^]=  { 119 }
^+^+^+^+_+^+^]=  { 111 }
_+^+_+_+^+^+^]=  { 114 }
_+_+^+^+_+^+^]=  { 108 }
_+_+^+_+_+^+^]=  { 100 }
^+_+_+_+_+^+_]=  {  33 }
_+^+_+^+*]       {  10 }
```

## Official interpreter

To run a program: `boolx <source_file>` from the command line.
Program source files are simple text files with the courtesy `.bx` extension.
The interpreter reads Unix (LF) line endings.

The interpreter features a debug mode activable with the `-d` option which makes it easier to understand what's going on during runtime.

## Compactor utility

With the utility software [compactorx](src/compactor.c) it's possible to remove comments and compact a program with the goal of creating an artistic and esoteric source code.

## Overview

_BoolX_ works with an array of infinite cells. Every cell can contain from zero to infinite bits.
Bits are stored from the least significant bit to the most significant bit.
Bits can be either _zero_ or _one_, but they can also be _null_ (deleted from memory) to indicate the end of the value: if a bit is _null_, every other bit from there to the most significant bit will also be _null_ (deleted from memory).

At the start of a program every cell has a _null_ value.

The array cursor starts from the first cell. The program can change the value of the cell by manipulating its bits, or it can move to other cells.

_BoolX_ uses [if-else statements](#if-else-statements) in order to simplify writing complex programs.

_BoolX_ allows using [functions](#labels-jumps-and-functions), which when called create a new array of infinite cells. The main program is considered a function.
The [global queue](#the-global-queue) can be used to store and retrieve values from any point of the program.

The main program starts from the first character of the file. Of course it's possible to create a function intended as _"main"_ and call it at the start of the source. If at the end of some function there is code that's not supposed to be executed, the function must terminate with `~`; otherwise there's no need to and the program will terminate once the _EOF_ is reached.

Only recognized symbols are processed, other symbols are ignored; however it's recommended to enclose comments between `{` and `}` for readability and future compatibility.

## Instructions
| Symbol | Effect |
| ------ | ------ |
| `>`	| go to the next cell |
| `<`	| go to the previous cell |
| `\|`	| go to the first cell |
| `+`	| select the next bit of the current cell |
| `-`	| select the previous bit of the current cell |
| `=`	| select the first bit of the current cell |
|  |  |
| `_`	| set the selected bit of the current cell to _0_ |
| `^` |	set the selected bit of the current cell to _1_ |
| `*`	| set the selected bit and the following bits (to the most significant bit) of the current cell to _null_ |
| `%`	| set all the bits of the current cell to _null_ and go to the first bit |
|  |  |
| `]`	| print the ASCII character corresponding to the current cell value |
| `[`	| get an ASCII character from the user and save its corresponding binary value to the current cell |
|  |  |
| `#`	| enqueue in the global queue the value (the bits) of the current cell |
| `&`	| dequeue a value from the global queue and save it to the current cell; `=` is implicit |
|  |  |
| `?`	| _if condition_ which tests if the current bit is equal to _1_; for _0_ just have an _else condition_ immediately after: `?!` |
| `"`	| _if condition_ which tests if the current bit is _null_ |
| `!`	| _else_ condition |
| `;`	| end of an _if statement_ or an _if-else statement_ |
|  |  |
| `:`	| set a label; its position will be registered in a dedicated array before execution of the code |
| `/`	| move the global label cursor to the next global label cell |
| `\`	| move the global label cursor to the previous global label cell |
| `$`	| move the global label cursor to the first global label cell |
| `'`	| jump to the label pointed to by the label cursor (basically a _goto_) |
`@`	| call a function jumping to the label pointed to by the label cursor (`~` will then return) |
|  |  |
| `~`	| terminate the function / main program |
|  |  |
| `{`	| start of comment (comments can be nested) |
| `}`	| end of comment |

## Comments

How they work:
```
{ NOT run } run
{ { NOT run } NOT run
NOT run } run
```

## Labels, jumps, and functions

Labels are declared with `:`. Once the interpreter is launched, it searches the whole program source file for labels and it saves their position in a designated **global** array which can be traversed with `/`, `\`, and `$`.

```
:
{ some code here that it's possible to jump to }
```
To jump to a label, the label cursor must first be moved with `/`, `\`, or `$`, then a jump to it can be made.
The same label can be both a destination of a jump (executed with `'`) and the entry point of a function (called with `@`); in this last case the function must terminate with the return instruction `~` or with the _EOF_.

Jumps break out from every _if condition_.

It's allowed to jump to a label that doesn't belong to the current function; this won't make any additional change to the memory (the function is still considered running) and it's a practice that must be used with caution.

Jumping to a label:
```
/// { move the label cursor to the fourth pointer }
'   { jump to the position in the program described
      by the fourth label pointer }
```

Function declaration and implementation:
```
:
{ code of the function here }
~ { terminate the function and return to the caller }
```

Calling a function:
```
$  { move the label cursor to the first label pointer }
/  { move the label cursor to the second label pointer }
@  { call the function starting from the position in
     the program described by the second label pointer }
```

By calling a function, a new array of cells is created. This array can be accessed only from the function itself and the previous array(s) cannot be accessed. Therefore, in order to pass values from function to function, the [global queue](#the-global-queue) must be used.


## If-else statements

By typing `?` or `"` an _if condition_ starts, and all the instructions wich follow are only processed if the condition is satisfied. The _if statement_ is terminated with `;`. It's also possible to add an _else statement_ with `!`; then, the _if-else statement_ can be terminated with `;`.

Examples:

`?_;     { if the bit is equal to 1, make it 0 }`

```?_!*;   { if the bit is equal to 1 make it 0,
           otherwise make it null }
```

`^:?]'   { infinite printing }`

```
=? +    { first IF condition: if the first bit is
	{ equal to 1: }
  ? * ; { a secound IF condition is done on the next bit;
          if it's equal to 1, make it null }
  ! > ; { otherwise go to the next cell }
 ;      { end of the first IF condition }
```

## The global queue

The global queue is a global FIFO structure that can be accessed from any function.
It is useful to do operations quickly (for example as a support array) or to pass values from function to function.
It can contain infinite cells with values of infinite length.
The content of the cells cannot be directly manipulated: dequeueing and queueing are the only possible operations.

Using a function and the global queue:
```
{ main program }
[>[< { get two values from the user }
#>#  { enqueue the values into the global queue }
@    { call the function below }
|    { go to the first cell }
&>&  { overwrite the first two cells with the new values
       dequeued from the global queue }
<]>] { print the two values }
~    { end of the main program; without this the
       following function will be processed again }

:    { a function }
&>&  { read values from the global queue}
|^   { change the first byte of the first value to 1 }
>_   { change the first byte of the second value to 0 }
|#># { put the two new values back into the global queue }
~    { terminate the function and return to the main
       program; this symbol is not necessary if EOF
       follows }
```

As explained in the last comment, the program never ends until the last function (including the main program) finishes the execution.
