# Code style guidelines for the BoolX programming language

## General

Keep general purpose functions in the _libraries_ folder, fully commented.

Include them (manually copy them) in your main program compressed and uncommented (except the function declaration/definition comments).

Use comments with pseudo-code to describe what's going on:

```
[		{ get(x) }
>^		{ int y = 1 }
```

Those are tabs. _x_ and _y_ are pseudo variable names.

## Labels

`  : { (4) add_loop: }`

where 4 means it's the fourth label in the program.

No tabs here.

## Jumps

`'		{ goto label_name }`

## Pointing to a label

`$//		{ *label_name }`

## Functions

```
: { (1) int add(int x, int y) }
  {     internal labels: 1 }
  {     perform an addition between two values and return the result }
```

where `(1)` is the number of the label, `int add...` is the pseudo declaration/definition of the function, `internal labels: 1` tells that inside this function there is _1_ other label; this allows to know how many `/` instructions to type to reach e.g. the next function.

The last line is the description of the function.

Below there should be the body of the function, each line starting with two spaces.

## Function calls

`#>#@<&<	{ bits_counter = sub(bits_counter, the_number_1) (bits_counter--) }`

where `bits_counter` is the cell that will get the dequeued value, `sub` is the function, `bits_counter` and `the_number_1` are the parameters, that is the cells that will get queued.

`(bits_counter--)` is just additional pseudo-code to make even clearer what's going on, in this case subtracting _1_ from _bits_counter_.

## Description of the memory

Both in the main program and in functions, after the first cells initializations, open a bracket and list what the cells are for:

```
&>_>^>^+_+_+_+^+^=>_+_+_+_+^+^=|
{ cell #0: int x
  cell #1: int bits_counter = 0
  cell #2: int the_number_1 = 1
  cell #3: ASCII character '1'
  cell #4: ASCII character '0'
}
```

## If-else statements

```
" >
  " >
    ? >^#~
    ! >#~ ;
  ! < ;
;
>>
```

This is easy to read, because it's like:

```
if null { >
          if null { >
                    if 1 { > ^ # ~ }
                    else { > # ~ }
                  }
          else { < }
        }
> >
```

