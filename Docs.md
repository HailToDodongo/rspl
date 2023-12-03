# RSPL - Documentation
<a href="../../Readme.md" target="_blank">Download Markdown</a>

This project provides tools for a high-level RSP language which can be transpiled into assembly in text form.<br>
The output is a completely functional overlay usable by libdragon.

RSPL is intended to be a simple language staying close to the hardware/ASM instructions, while proving a GLSL-like "swizzle" syntax to make use of the vector/lane instructions on the RSP.

While it may look like C / GLSL, the syntax/behaviour is quite different.<br>
Here are a few noteworthy differences:
- Variables only exist in registers
- Memory access is explicit (e.g. with the use of `load()` / `store()`)
- Statements only allow one "computation" (`a = b + 2` is ok, `a = (b * 2 ) + 4` is not)
- Computations have no inherent value, they always assign to a variable in the end.
- Builtin functions are aware of the whole expression, allowing for "return-type overloading"

Further details can be found in the following sections.

## File Layout

An RSPL file contains three sections, an optional state section at the start,<br>
includes, and zero or more functions after that.

The state section can allocate memory which gets saved across overlay-switches.<br>
While functions can implement the actual code and register themselves as commands to libdragon.

An include, currently only for raw assembly files, can be added to the very beginning or end of a file.

This is what a simple file could look like:
```C
include "rsp_queue.inc"

state {
  vec16 SCREEN_SIZE_VEC;
}

command<0> T3DCmd_SetScreenSize(u32 unused, u32 sizeXY)
{
  u32 sizeX = sizeXY >> 16;
  u32 sizeY = sizeXY & 0xFFFF;

  vec16 screenSize;
  screenSize.x = sizeX;
  screenSize.y = sizeY;
  store(screenSize, SCREEN_SIZE_VEC);
}

```

## Types
Both the state definition and variables in the actual code have to declare their types, similar to C.<br>
There is currently no way to define custom or compound types (e.g. structs).<br>
Types can be put into two categories: scalar-types and vector-types.<br>

This means that there is a strict separation of scalar and vector instructions, except statements that are used to transfer data between the two.

Here is a list of all available types:
- Scalar: `u8`, `s8`, `u16`, `s16`, `u32`, `s32`
- Vector: `vec16`, `vec32`

(@TODO: implement signed/unsigned/fraction vector types)

Like with C, the emitted instructions for operations are dependent on the types involved.

### `vec32`
One special case is the type `vec32`, which is assumed to be a 16.16 32-bit vector.<br>
This means it's the only type split across two registers, where the first one contains the integer part and the second one the fractional part.<br>
Any operation with that type takes this into account, any special behaviour will be documented.

## State
In order to have memory that persists across overlay switches, a state section has to be defined.<br>
This is done by using the `state` keyword, followed by a block inside curly-brackets.<br>
Inside the block, labels with types and an optional array-size can be declared.<br>

As an example:
```c++
state {
  extern u32 RDPQ_CMD_STAGING;
  
  vec32 MAT_MODEL_DATA[8][2];
  u32 CURRENT_MAT_ADDR;
}
```
As seen in the first entry, it's possible to declare external labels, which will be resolved at compile-time.<br>
This can be used for dependencies defined in assembly files (e.g. from `rsp_queue.inc`).<br>
External labels don't take up any extra space in the overlay.

## Variables
To access registers, variables must be used.<br>
You can think of them as just an alias for registers, they have no runtime overhead.<br>
Additionally they provide the ability to have transpile-time checks.

You can declare a variable like that:
```c++
u32<$t0> myVar;
u8 alsoMyVar;
vec32 aVectorVar;
```
In the declaration it's possible to either specify a register directly (useful for function-calls with arguments), or let the transpiler choose one.<br>
The latter should be preferred whenever possible.<br>
Automatic allocation of registers happens in a fixed order, choosing the first free register.

### Scope
RSPL has a concept of scopes which, similar to C, refers to a block of code inside curly-brackets.<br>
This only limits the visibly/lifetime of variable declarations.<br>
There is no runtime overhead or cost associated with scopes.

As an example:
```c++
u32 varA;
{
    u32 varB;
    varA = 42; // <- OK
}
varB = 42; // <- Error, no longer in scope
```

The main use is to efficiently manage registers, since once out of scope, they are free to be re-used again.
Note that calling a macro (explained later), creates an implicit scope for that exact reason.

## Statements
Code consists of a collection of statements.<br>
A Statement can be a variables declaration, function call, calculation or control-structure.<br>
As an example:
```c++
u32 a, b, address;
a = 10;
b = a + a;
if(a > 10) {
    a = load(address);
}
```
Nothing surprising here, just like with most other languages.<br/>
There is however a limitation: you can only write one expression/calculation per statement.<br/>
For example:
```c++
u32 a = b + 5; // <- OK
a += 3;        // <- OK  
a = load(b);   // <- OK

u32 a = b + 5 + 2; // <- ERROR
a += b + 3;        // <- ERROR
a = load(b) - 2;   // <- ERROR
```
By extension, grouping calculations in parentheses is also not allowed.<br>

For control-flow, there are `if`-statements and `while`-loops.<br>
They also only support one expression for handling the condition.<br>
Examples:
```c++
u32 a;

if(a > 10)a = 10;

if(a != 10) {
    a = 10;
} else {
    a = 20;
}

while(a > 20) {
  a -= 1;
}
```

Labels in combination with `goto` are supported too:<br>
```c++
u32 a;

LabelA:
a += 10;
goto LabelA;
```

### Swizzle
For vector types, there is a syntax-feature called "swizzle" which you may know from GLSL.<br>
This allows to specify one or more lanes/components of a vector.<br>
What swizzle you can actually use is heavily dependent on the statement, this is a hardware limitation.<br>
There are however error messages when a swizzle is incompatible with a certain statement.

One main difference to GLSL is that vectors always have 8 components, not 4.<br>
This is because the RSP uses 8 lanes for its registers.<br>
They are referred to, in order, as `x`, `y`, `z`, `w`, `X`, `Y`, `Z`, `W`.<br>
Depending on the statement, you can use specific combinations of those:<br>
- `xy`, `zw`, `XY`, `ZW`
- `xyzw`, `XYZW`
- `xxzzXXZZ`, `yywwYYWW`, `xxxxXXXX`, `yyyyYYYY`, `zzzzZZZZ`, `wwwwWWWW`
- `xxxxxxxx`, `yyyyyyyy`, `zzzzzzzz`, `wwwwwwww`
- `XXXXXXXX`, `YYYYYYYY`, `ZZZZZZZZ`, `WWWWWWWW`
- `xyzwxyzw`

As an example:
```c++
vec32 a,b,c;
a.y = 1.25; // assign 1.25 as a fixed-point to the second lane
a.x = b.z; // single-lane assignment

// This does: b: xyzwXYZW *
//            c: yyyyYYYY
a = b * c.yyyyYYYY;

u32 address;
store(a.xy, address); // stores the first two lanes to memory
```

### Cast
RSPL comes with a unified cast and partial-access syntax, specified by a colon (`:`) and type.<br>
This works for both scalar and vector types.<br>

#### Scalar
You can treat scalar values as different types, for example during a load and store:
```c++
u32 a, address;
a:u8 = load(address); // load unsigned 8-bit value
store(a:u16, address); // store back as 16-bit value
```
Allowed cast types: `u8`, `s8`, `u16`, `s16`, `u32`, `s32`

#### Vector
For vector types, this allows you partially access the integer or fraction part of a `vec32`.<br>
Using it on a `vec16` is also safe, and usually affects how it will be treated by 32-bit operations and assignments.<br> 

Allowed cast types: `sint`, `uint`, `sfract`, `ufract`.

For example, here is a partial load and store:
```c++
u32 address;
vec32 res;
res:sint = load(address); // load only integer part, fraction stays intact
store(res:ufract, address); // store only fraction part
```
This can also be combined with swizzling:
```c++
u32 address;
vec32 res;
res:sint.xy = load(address).xy; // load only first two lanes of integer part
store(res:ufract.XY, address); // only store upper two lanes of fraction part
```
Assignment can also make use of it, in all combinations:
```c++
vec32 a, b;
a:sint = b; // assign only integer part, leave fraction intact
a = b:sint; // assign only integer part, zero out fraction
a:ufract = b:ufract; // assign fraction to fraction, leave integer intact
```
As well as operations:
```c++
vec32 a, b;
a += b:sint; // only add an integer, leave fraction unchanged
vec16 fA, fB; // assumed to be a 1.16 fixed-point
fA += fB:sfract; // only add fraction, forcing a singed addition
```
While you can set a cast on all variables of an expression (if supported),<br/>
the main deciding factor is the destination variable.<br/>
If the destination variable has a cast, but one of the source variables doesn't, it will add a cast automatically.<br/>
For example:
```c++
vec16 a, b;
a:sfract = a * b; // assumes that 'a' & 'b' are 'sfract' too. (using 'vmulf')
a:ufract = a * b; // assumes that 'a' & 'b' are 'ufract' too. (using 'vmulu')
```

As a shorthand, a cast can be specified at a variable declaration if it's using a calculation.<br>
For example:
```c++
vec16 a, b;
vec16 varA:sfract = a * b; // <- OK, assumes sfract multiplication
vec16 varB:sfract; // <- ERROR
``` 

## Functions
Functions exist in 3 different forms, specified by a keyword: `function`, `command`, `macro`.

### `function`
A `function` is what you would expect: it's a section of code starting after a label (same as the function-name).<br>
At the end it will do a `jr $ra` to return to the caller.<br>
It can be called directly like in C, for example: `test();`.<br>
Functions can contain arguments, they are however purely a hint for the transpiler, as they don't actually do anything assembly-wise.<br>
Writing code like this `function test(u32<$t0> a){}` is the same as writing `function test(){ u32<$t0> a; }`.<br>
The first version should be preferred however, as it allows for checks if the arguments "passed into" the function have matching registers.<br>
For example the call
```c++
u32<$t5> b;
test(b);
```
with the function from before would throw an error, since the register doesn't match up.<br>
Same as for the definition, passing a variable into a function doesn't do anything other than a check at transpile-time.

One exception to this is if you pass a literal value into a function (only works for scalars).<br>
This will cause the value to be loaded into the register first, using the shortest amount of instructions.<br>

You are also able to declare external function by not providing an implementation.<br>
This is intended to bridge the gap between RSPL and code in assembly.<br>
For example:
```c++
function RDPQ_Send(u32<$s4> buffStart, u32<$s3> buffEnd);
```

### `command`
Commands are mostly identical to functions.<br>
They do however cause the function to be registered as an command in the overlay.<br>
For that, you need to specify the command ID/index like so: `command<4> my_command() { ... }`. <br>
The other difference is that the jump at the end will return to libdragons command queue.<br>

As for arguments, the first 4 can omit the register declaration since they are assumed to be `$a0` to `$a3`.<br>
Anything beyond that needs a register, and will cause an instruction to be emitted which loads them from memory.<br>
A command could look like this:
```c++
command<3> Cmd_MatrixRead(u32 _, u32 addressMat)
{
  dma_out(MAT_PROJ_DATA, addressMat);
}
```

### `macro`
Macros are again similar to functions, they are however always inlined.<br>
Meaning any "call" will in actuality just copy-paste their contents into the place where it was called from.<br>
This happens in its own scope, so any new variable will only be valid inside the macro.<br>
It will honor the register-allocations of the caller.<br>
Since it's inlined, arguments don't have a defined register, and will use whatever is passed in.<br>

Using macros should be preferred when possible to avoid jumps, and to have better register allocations.<br>
A macro could look like this:
```c++
macro loadCurrentMat(vec32 mat0, vec32 mat1, vec32 mat2, vec32 mat3)
{
  u32 address = load(CURRENT_MAT_ADDR);
  mat0 = load(address, 0x00).xyzwxyzw;
  mat1 = load(address, 0x10).xyzwxyzw;
  mat2 = load(address, 0x20).xyzwxyzw;
  mat3 = load(address, 0x30).xyzwxyzw;
}
```

## Scalar operations
The following operations are available for scalar types:
- Arithmetic: `+`, `-`, `*`, `/`
- Bitwise: `&`, `|`, `^`, `~`. `<<`, `>>`
- Assignment: `=`

Note: `*` and `/` is only supported with `2^x` contants (which use a shift instead).<br>
This is a hardware limitation.<br>
The shorthand operators for all (e.g. `+=`) are also available.

Inside `if`-statements, you have to use one of the following comparison-operators:
- `==`, `!=`, `>`, `<`, `>=`, `<=`

Example:
```c++
u32 a, b, c;
a = b - c;
a = b * 2;
if(a > 10) {
    c += 10;
} else {
    c = a ^ b;
}
```

## Vector operations
The following operations are available for vector types:
- Arithmetic: `+`, `-`, `*`, `+*`, `/`
- Bitwise: `&`, `|`, `^`, `~`
- Assignment: `=`
- Compare: `<`, `>=`, `==`, `!=`

Note: Division is very expensive, since it will be converted to a multiplication with the inverse.<br>
If you need the inverse, look at the `invert_half()` builtin.<br>

Due to the hardware using an accumulator for multiplication, there is a special operator `+*`.<br>
This keeps the accumulator intact, allowing for multiple mul.+add. in a row.<br>
As an example, you can use this to perform a matrix-vector multiplication:<br>
```c++
macro matMulVec(vec32 mat0, vec32 mat1, vec32 mat2, vec32 mat3, vec16 vec, vec32 out) {
  out = mat0  * vec.xxxxXXXX;
  out = mat1 +* vec.yyyyYYYY;
  out = mat2 +* vec.zzzzZZZZ;
  out = mat3 +* vec.wwwwWWWW;
}
```
For basic operations, the usage is identical to scalars.<br>

### Compare
In contrast to scalars, vectors have special comparison operators, only usable outside of `if`-statements.<br>
These act like simple ternary/select instructions by first comparing the two vectors, and then storing the matching value in the destination.<br>
To use different values than the ones used in the comparison, you can use the `select()` builtin.<br>
Internally, comparisons keep the result stored in the `VCC_LO` register.

Examples:
```c++
vec16 res, a, b;
// if true, stores 'a' into 'res', otherwise 'b'
res = a < b;
// constant values (0 or 2^x) are also allowed
res = a >= 32;

// full ternary:
vec32 x, y;
res = a != b; // (result can be ignored here)
res = select(x, y); // uses 'x' if a != b, otherwise 'y'
```

### Swizzle
Some operators can make use of swizzling, allowing for more complex operations.<br>
You can however only use swizzling on the right-hand side of an operation.<br>
This is a hardware limitation.<br>
For Example:
```c++
vec16 a, b, mask;
// masks the first 4 lanes of 'a' against only the first lane of 'mask', same for the last 4 lanes
a &= mask.xxxxXXXX;
a = b + b.x; // adds the first lane of 'b' to all lanes of 'b', storing the result in 'a'
```

## Builtins
RSPL provides a set of builtin macros, usually mapping directly to instructions.<br>
Here is a list of all builtins:

### `dma_in(dmem, rdram, [size])` & `dma_in_async(...)`
Performs a DMA transfer from RDRAM to DMEM.<br>
The `size` parameter is optional if the first argument is a label from the state section.<br>
Using `dma_in` will wait for the transfer, while `dma_in_async` will return immediately.<br>

### `dma_out(dmem, rdram, [size])` & `dma_out_async(...)`
Performs a DMA transfer from DMEM to RDRAM.<br>
The `size` parameter is optional if the first argument is a label from the state section.<br>
Using `dma_out` will wait for the transfer, while `dma_out_async` will return immediately.<br>

### `dma_await()`
Waits for all DMA transfers to be finished.<br>
This uses libdragon's `DMAWaitIdle` function.

### `swap(a, b)`
Swaps two scalar or vector values in place.
Examples:
```c++
u32 a, b; vec16 v0, v1;
swap(a, b); // swap two scalars
swap(v0, v1); // swap two vectors
```

### `select(vec a, b)`
Selects between two vectors, based on the result of the last comparison.<br>
This can be used to implement a ternary operator.<br>

Examples:
```c++
vec16 a, b;
vec32 res, dummy;

dummy = a < b; // compare, result doesn't matter here
res = select(a, b); // 'a' if last comparison was true, otherwise 'b'
res = select(a, 32); // constants are allowed too
```

### `invert_half(vec a)` & `invert(vec a)`
Inverts a (single component of a) vector (`1 / x`).<br>
The `invert_half` version maps directly to the hardware instruction, returning `0.5 / x`.<br>
`invert` already multiplies the result by 2.

Example:
```c++
vec32 pos;
posInv.w = invert_half(pos).w;
```

### `invert_half_sqrt(vec a)`
Inverted square-root a (single component of a) vector (`1 / sqrt(x)`).<br>

Example:
```c++
vec32 pos;
posSqrtInv.w = invert_half_sqrt(pos).w;
```

### `load(u32 address, offset, ...)`
Loads a value from memory `address` into a register.<br>
Examples:
```c++
u32 address;
u32 v = load(SOME_STATE_LABEL); // load 32-bit scalar
u8 v = load(SOME_STATE_LABEL); // load 8-bit scalar
vec16 a = load(address, 0x10); // loads entire 16-bit vector from address + 0x10
vec32 a = load(address); // loads a 32-bit vector
vec16 b = load(address, 0x10).xyzwxyzw; // loads the first for lanes, repeating them

vec32 c; // only load the first 4 lanes
c.xyzw = load(address, 0x00).xyzw;

// load entire vector, writing into the register starting from lane 5
c.Y = load(address, 0x00);
```
There is some special behaviour for`vec32`:<br>
The assumed memory-layout is that the fractional part follows the integer part after each register.<br>
Meaning it should look like this (two bytes per letter): `IIII IIII FFFF FFFF`.<br>
If you specify a partial or repeating load (e.g. `.xyzw`, `.xyzwxyzw`)  it should look like this: `IIII FFFF`.<br>

In general, the swizzle will specify the amount of components and the target offset.<br>
The fractional part always comes after that block.

### `store(value, address, offset)` 
Stores a value from a register into memory.<br>

Similar to `load()`, there is some special behaviour for vectors.<br>
Swizzling will work in the same way as for `load()`, also assuming the same memory layout.<br>
However it has to be set in the argument.<br>

Example:
```c++
u32 address, value;
vec16 uv;
store(value, address); // store 32-bit scalar
store(value, address, 0x10); // store 32-bit scalar at address + 0x10
store(uv, address); // stores entire vector to address
store(uv.xy, address, 0x0C); // stores the first two lanes to address + 0x0C
store(uv.XYZW, address, 0x0C); // stores the last 4 lanes to address + 0x0C
```
### `load_vec_u8(address, offset)` & `load_vec_s8(...)`
Special load for packed 8-bit vectors, using the `lpv` / `luv` instructions.<br>
Instead of loading 16-bit per lane, it loads 8-bit per lane expanding the value.<br>
This only accepts `vec16` variables, and an `ufract`/`sfract` type should be assumed.<br>
Example:
```c++
vec16 color = load_vec_u8(ptrColor, 0x08);
```

### `store_vec_u8(valuem, address, offset)` & `store_vec_s8(...)`
Special store for packed 8-bit vectors, using the `spv` / `suv` instructions.<br>
Instead of storing 16-bit per lane, it stores 8-bit per lane scaling the value.<br>
This only accepts `vec16` variables, and an `ufract`/`sfract` type should be assumed.<br>
Example:
```c++
vec16 color;
store_vec_u8(color, ptrColor, 0x08);
```

### `asm(x)`
Injects raw asm/text into the output, no checks are performed<br>
Example:
```c++
asm("sll $a1, $s5, 5"); 
```

## References
- <a href="https://emudev.org/2020/03/28/RSP.html" target="_blank">RSP Instruction Set</a>