# Notes

This was my CSE340 - Principles of Programming Project 3. The language it compiles is made up.
This was made completely in C++. 

# The Constraints

-C++

-Must use inputbuf.cc, lexer.cc, and compiler.cc

-Must implement demo.cc

# What I Would Of Done Differently

-Make head and last global (to allow complex multi-instruction chaining)

-Make a function to reuse parsing segments that show up multiple times (conditions or assignments)

-Parsing inputs was over-engineered as my design approach switched mid-programming


# Examples

Here are some samples of what it can compile:

Assignment, input, output
```
i, j;
{
	input i;
  	i = 42 ;
  	j = i + 1; 
	output i;
	output j;
}
1 2 3

```

While Loops
```
a, b, c, i ;
{
a = 12345;
b = 6789;
i = 0;


c = b;
WHILE c > 0 {		
	c = c/10;
	i = i+1;
}


WHILE i > 0 {
	a = a*10;
	i = i-1;
}

a = a+b;
output a;
}
2 3 4 1
```

Switch, Case, and Default
```
a, b, c, d;
{
    input a;
    input c;
    SWITCH 1 {
        CASE 0 : {
            output b;
            output b;
        }
        CASE 1 : {
            output a;
            output a;
        }
        DEFAULT : {
            output c;
            output c;
        }
    }
}
1 2 3 4
```

If Statements (Greater Than, Less Than, Not Equal)
```
a , b ;
{
a = 1  ;
b = 2 ; 
IF  a <> b
{	output b;
	output a;
	a = a + b; 
	output a;
	b = a + b;
	output b;
}
input a;
a = a + b;
output a;

}
2 3 4 6
```