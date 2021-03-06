/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31.


EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */

#endif
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
/* 0 0 -> 0
   0 1 -> 1
   1 0 - > 1
   1 1 - > 0
*/
// ~(~a & ~b)&~(a&b)
int bitXor(int x, int y)
{
  int ans = ~(~x & ~y) & (~(x & y));
  // printf("%d\n",ans);
  return ans;
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
/*
  0X10000000
*/
int tmin(void)
{

  int ans = 0x1 << 31;
  return ans;
}
//2
/* 
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 *   0x7FFFFFFF
 */
int isTmax(int x)
{
  /*
  大体思路首先是根据，如果x是最大值0x7FFFFFFF,那么~x和x+1(自然溢出)应该相等。
  不能用等号，但是我们可以用异或。x==y 等价于  !(x^y). 因此有了后半段!(x+1)^(~x)
  但是满足这个条件的还有-1,也就是0xFFFFFFFF,因此我们需要排除掉-1.
  还是用异或的性质，这回是0异或者任何数都等于其本身。
  因此如果x为-1，那么前后两部分都为1，结果为0.
  如果x为TMAX,那么前面为0，后面为1，结果为1.
  如果x为其他任何数，前后结果都应为0. 结果为0。
  */
  return (!(x + 1)) ^ !((x + 1) ^ (~x));
}
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
// 理解错误。误以为是要求当前长度x的所有奇数位上都是1.
// 实际上要求和x的长度无关，而是要求[0,31]中，所有奇数位上都是1.
int allOddBits(int x)
{
  int half_mask = (0xAA << 8) | 0xAA;
  int mask = (half_mask << 16) + half_mask;
  // printf("mask:%08X x:%08x %08x\n",mask,x,x&mask);
  return !((x & mask) ^ mask);
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x)
{
  return ~x + 1;
}
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
/* 把0-9的二进制写出来，发现0-7占满了3bit的二进制的8种组合。
   因此考虑只判断8和9两种4bit的情况

   构造mask,不在意的bit的位置放0，在意的bit位置放1.
*/
int isAsciiDigit(int x)
{
  int mask = 0x0E;
  int ones = x & mask;
  int ones_3 = ones >> 3;
  int tens = x >> 4;
  // printf("x: %08x tens: %08x ones:%08x\n",x,tens,ones);
  int ones_ok = (!(ones ^ 0x8)) | (!ones_3);
  int tens_ok = !(tens ^ 0x3);
  return ones_ok & tens_ok;
}
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
/*
  关键思路是0xFFFFFFFF和0x00000000之间差了1.
  而这两个数一个是全部位置都取的mask,一个是全部位置都不取的mask.

*/
int conditional(int x, int y, int z)
{
  return z ^ (!x + ~0) & (y ^ z);
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
/*
  大体思路是，符号位相同和符号位不同分别考虑
  符号位相同:  考虑差的符号位。
  符号位不同: 当x<0,y>=0时结果为1.
*/
int isLessOrEqual(int x, int y)
{
  int minus = y + (~x + 1);
  int s_x = (x >> 31) & 1;
  int s_y = (y >> 31) & 1;
  int s_minus = (minus >> 31) & 1;
  return (s_x & (!s_y)) | (!(s_x ^ s_y) & !s_minus);
}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
/* 
  0 == ~0+1
  -2147483648 = ~-2147483648+1
  满足 x == ~x+1
  重点是x和~x+1的符号位相同，如果都是0那么x=0,如果都是1那么x=-214783648`
*/
int logicalNeg(int x)
{
  int s1 = (x >> 31) & 1;
  int s2 = ((~x + 1) >> 31) & 1;
  // printf("s1: %d s2:%d  %d  %d\n",s1,s2,s1|s2,~(s1|s2));
  //  1 + negate(0) -> 1
  //  1 + neagate(1) -> 0
  return 1 + (1 + ~(s1 | s2));
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1 ??? should be 2?
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
/*
  思路似乎可以转化成判断一个数（可正可负）的最高位的1的位置。
  判断最高位1用二分的办法。
  构造一个单调的函数，假设最高位位置为a,那么f((a,32))=0,f([0,a])=1.
  被 howManyBits(-1)==1 困扰了好久，实际上就是0x1，只有一位，改位就是符号位的情况。 

*/
int howManyBits(int x)
{
  int n = 0;
  x ^= (x << 1);
  n += (!!(x & ((~0) << (n + 16)))) << 4; // 看高16位是否为0，是的话区间为[0,16),否的话为[16,32)
  // printf("n:%d\n",n);
  // printf("%d\n",!!(x & ((~0) << (n + 16))));
  n += (!!(x & ((~0) << (n + 8)))) << 3;
  // printf("n:%d\n",n);
  n += (!!(x & ((~0) << (n + 4)))) << 2;
  // printf("n:%d\n",n);
  n += (!!(x & ((~0) << (n + 2)))) << 1;
  // printf("n:%d\n",n);
  n += (!!(x & ((~0) << (n + 1))));
  // printf("n:%d\n",n);

  // int s = (x>>31)&1;
  // int ret = n+1+((1^s)&(!!x));
  // // printf("x:%d ret:%d\n",x,ret);

  return n + 1;
}
//float
/* 
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale2(unsigned uf)
{
  int exp_ = (uf & 0x7f800000) >> 23;
  int s_ = uf & 0x80000000;
  if (exp_ == 0)
    return (uf << 1) | s_;
  if (exp_ == 255)
    return uf;
  ++exp_;
  if (exp_ == 255)
    return 0x7f800000 | s_;
  return (uf & 0x807fffff) | (exp_ << 23);
}
/* 
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int floatFloat2Int(unsigned uf)
{
  int s_ = uf >> 31;
  int exp_ = ((uf & 0x7f800000) >> 23) - 127;
  int frac_ = (uf & 0x007fffff) | 0x00800000;
  if (!(uf & 0x7fffffff))
    return 0;

  if (exp_ > 31)
    return 0x80000000;
  if (exp_ < 0)
    return 0;

  if (exp_ > 23)
    frac_ <<= (exp_ - 23);
  else
    frac_ >>= (23 - exp_);

  if (!((frac_ >> 31) ^ s_))
    return frac_;
  else if (frac_ >> 31)
    return 0x80000000;
  else
    return ~frac_ + 1;
}
/* 
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 30 
 *   Rating: 4
 */
unsigned floatPower2(int x)
{
  int exp = x + 127;
  if (exp <= 0)
    return 0;
  if (exp >= 255)
    return 0x7f800000;
  return exp << 23;
}
