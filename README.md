
# 👁‍🗨blink



Welcome to 👁‍🗨**blink**



This is a simple high level non-turing complete programming language, based on the YAML schema.

It can be used for data passing, IO, and library manipulation. 

Key features:

- YAML compliant
  
- Libraries

- Non-turing complete

- Subroutines and Subconstants
  
- Multiple translation units

- Dynamic type system and coercion

- Exceptions


### YAML Compliance:

👁‍🗨**blink** supports standard YAML compliance, with built in AOT schema validation, to ensure errors can be caught quickly and effectively.

```yaml
# Pure YAML syntax applies seamlessly.
# Comments, lists, and key-value pairs are all parsed natively.
my_list:
  - item1
  - item2
```

### Libraries

👁‍🗨**blink** allows importing libraries, using `import` anywhere in the programme. The default library packaged with 👁‍🗨**blink** is `std`

```yaml
# Import standard library to gain access to IO and math operations
import: [std]

main:
  # std.writeout is a special sub, since it accepts any number of arguments 
  - std.writeout: ["Hello %i!", 42]
```

👁‍🗨**blink** libraries have strict naming conventions, they must be named `bk[name]lib`, so the `std` module is called `bkstdlib`.

### Non-turing complete:

👁‍🗨**blink** is NOT turing complete. The lack of decision control is an intentional design to ensure the halting problem is a non-issue, this means that, granted the libraries do not halt, any 👁‍🗨**blink** program will eventually terminate!

```yaml
import: [std]
main:
  # There are no 'if' statements or 'while' loops here.
  # Programs flow directly from top to bottom (or via bounded recursion).
  - std.writeout: ["This program will always halt!"]
  - True
```

### Subroutines and Subconstants

👁‍🗨**blink** has a very powerful concept of "subroutines" and "subconstants". Subconstants are simple versions of subroutines that are 100% pure, and only return a value; this is a clever way to declare constants.

A subroutine can be overridden in definition later on, dynamically as explained below. Subroutines return values by simply declaring values at the end of a function. A subroutine by default always returns `unknown`. 

```yaml
import: [std]
# Subconstant that returns 0, identical to a "variable"
zero:
  - 0
# This statement is equivalent!
zero: 0

# A subroutine that accepts arguments and increments a number by 1
inc:
  # declare expected arguments using the special `args` statement.
  # This must be declared in the top of the subroutine!
  - args: [x]
  # evaluates, and returns the value (since it's the last statement)
  - std.add: [x, 1]

main:
  # Calling a sub explicitly with 0 arguments, passing the value to `foo`
  - foo:
    - zero: []
  # this statement is also equivilant!
  - foo: zero
```

### Multiple translation units:

👁‍🗨**blink** allows to manipulate multiple translation units, to allow independent execution environments to operate, without mutating another, unless explicitly performed by using a subroutine.

```yaml
# Unit 1: data.bk.yaml
- num: 5

# Unit 2: main.bk.yaml
# Operates independently in its own execution environment.
# Since `num` is not declared here, a `UndeclaredSubException` will be thrown
import: [std]
main:
  - std.writeout: ["Running isolated unit", num]
```
### Dynamic scoping:

👁‍🗨**blink** is fully dynamically scoped. This is useful for manipulating subrotunines and subconstants, and enforces unique naming practices. Since these are locked within the translation unit, you do not need to worry about name collisions with other units!

```yaml
import: [std]
foo:
  # x is not defined! but with dynamic scoping, this will write "x = 2"!
  - std.writeout: ["x = %i", x]

main:
  # declaring x and setting it to 2 (dynamically scoping x)
  - x: 2
  # "x = 2" will be outputted!
  - foo: []
```

### Dynamic type system and coercion:

👁‍🗨**blink** similarly to Javascript is dynamically typed, but it also has excellent type coercion! This allows for types to easily be converted trivially one to another, with added benefits of representing data in high levels. The types allowed in 👁‍🗨**blink** are:

  - Object: arbitrary data defined by a library, each object must explicitly implement the type coercion for all types below. 

  - Unknown: void data. Defaults to 0 for all types, implementation defined by Object

  - Boolean: a flag that represents truthy or falsy values, can be defined by `true y yes on` for truthy values and `false n no off` for falsy values. Note: these are case *insensitive*, meaning 'YES' and 'ofF' are perfectly valid. 

  - Integer: a 32 bit signed integer.

  - Decimal: a 32 bit floating point number.

  - String: a null-terminated UTF-8 string. Can be defined by "double quotation" or 'single quotation'

```yaml
# Variables are considered subroutines. They are immutable.
- u: unknown

# Booleans (truthy: true, y, yes, on | falsy: false, n, no, off)
- b1: True
- b2: Yes
- b3: On
- c1: false
- c2: no
- c3: off

# Integers and Decimals
- i1: 42
- f1: 3.1415926

# Strings
- cstr: "Hello"
- cstr2: '10'

# References (copying subconstants)
- cpy: b1
- cpy2: cpy

```

The type coercion is the strong point of this language, it has the power to convert types intuitively to others.  The default conversions are as follows:

  - Unknown: converts to `false`, `0`, `0.0`, `""`. 

  - Boolean: falsy values evaluate to `0`, `0.0`, `"false"`, and truthy to `1`, `1.0`, `"true"`

  - Integer: for Booleans, non zero numbers evaluate to truthy, and zero to falsy. For decimals, the value is expanded. For Strings, the Integer value is stringified.

  - Decimal: for Booleans, values between (-0.5, 0.5) (Exclusive) are evaluated as falsy, and the rest as truthy. For Integers, the value is always rounded to the nearest integer, this is to mitigate floating point errors. For strings, the Decimal value is stringified.

  - String: for Booleans, `""`, `"false"`, `"off"`, `"no"`, `"n", "0", "(from -0.5 exclusive to 0.5 exclusive)"` evaluate to falsy values, and everything else to truthy. For integers, strings will always assume the lowest available base first, if only numbers are used, these will be converted to integer using base 10 notation. Else, if alphanumeric characters in the range of `0-9` and `A-F` are used, the String is interpreted as a hexadecimal integer (base 16). Else, if alphanumeric characters in the range of `0-9` and `A-Z` are used, the string is converted following Base 36 notation. If this does not work, the string is assumed to follow Base64 conventions, and will convert each character to it's corresponding number and radix (Note that Base64 is *not* case insensitive). If all else fails, the result will be -1, indicating a conversion failure. Decimals follow a simpler approach, if the number does not evaluate to a Decimal, a `NotANumberException` is thrown. This is to prevent bad values like NaNs plaguing the programme.

***Example:***
```yaml
# Type Coercion Examples:
import: [std]
main:
  # 1. String to Integer coercion (Dynamic Base Parsing)
  - string_num: "FF" 
  # "FF" contains 'F', so it naturally coercions via Hexadecimal (Base 16) to 255
  - math_result:
    - std.add: [string_num, 5] # Result is implicitly 260
    
  # 2. Decimal to Boolean coercion
  - small_dec: 0.3
  - is_falsy:
    # Decimals strictly between -0.5 and 0.5 are falsy. 
    # Therefore, std.not will return True!
    - std.not: [small_dec] 

  # 3. String to Decimal coercion with Exception Fallback
  - bad_dec_str: "Not a number"
  - safe_parse:
    # Instead of silently failing with NaN, this throws NotANumberException
    # We catch it using the exception stack (see below!)
    - result:
      - catch: [NotANumberException]
      - std.mul: [bad_dec_str, 1.0]
    
```

### Exceptions

👁‍🗨**blink** has sophisticated exception handling. Exceptions are by default propagated up the coroutine chain. These can be caught by pushing a `catch: []` expression before using the sub, and will cause the next sub usage to return an Object that must be resolved using `std.ex.getmsg` to read the exception message, `std.ex.getcod` to read the exception code, `std.ex.unbox[b, i, d, s, o]` to get the underlying value. If an exception was thrown, the value will always return `unknown`.

```yaml
import: [std]
main:
  # Catching an exception by wrapping the subroutine call by pushing a `catch: [ExceptionCode (optional)]` 
  - error_obj:
    - catch: []
    - std.risky_operation: []
    
  # Extracting the message and code from the exception object
  - msg:
    - std.ex.getmsg: [error_obj]
  - code:
    - std.ex.getcod: [error_obj]
  - value:
  # Unboxing as an integer, if the exception has been thrown,
  # this will return 0 (As seen in the type coercion)
    - std.ex.unboxi: [error_obj]
    
  # Printing the caught exception message
  - std.writeout: ["Caught Exception: %s, value may be %i", msg, value]
```
