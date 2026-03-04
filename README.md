# 👁‍🗨blink

Welcome to 👁‍🗨**blink**

This is a simple high level non-turing complete programming language, based on the YAML schema.
It can be used for data passing, IO, and library manipulation. 
Key features:
- YAML compliant
- Non-turing complete
- Subroutines and Subconstants
- Multiple translation units
- Dynamic Scoping
- Dynamic type system and coercion
- Exceptions

### YAML Compliance:
👁‍🗨**blink** supports standard YAML compliance, with built in AOT schema validation, to ensure errors can be caught quickly and effectively.


### Non-turing complete:
👁‍🗨**blink** is NOT turing complete. The lack of decision control is an intentional design to ensure the halting problem is a non-issue, this means that, granted the libraries do not halt, any 👁‍🗨**blink** program will eventually terminate!

### Subroutines and Subconstants
👁‍🗨**blink** has a very powerful concept of "subroutines" and "subconstants". Subconstants are simple versions of subroutines that are 100% pure, and only return a value; this is a clever way to declare constants.
A subroutine can be overridden in definition later on, dynamically as explained below. Subroutines return values by simply declaring values at the end of a function. A subroutine by default always returns `unknown`. 

### Multiple translation units:
👁‍🗨**blink** allows to manipulate multiple translation units, to allow independent executionenvironments to operate, without mutating another, unless explicitly performed by using a subroutine.

### Dynamic scoping:
👁‍🗨**blink** is fully dynamically scoped. This is useful for manipulating subrotunines and subconstants, and enforces unique naming practices. Since these are locked within the translation unit, you do not need to worry about name collisions with other units!

### Dynamic type system and coercion:
👁‍🗨**blink** similarly to Javascript is dynamically typed, but it also has excellent type coercion! This allows for types to easily be converted trivially one to another, with added benefits of representing data in high levels. The types allowed in 👁‍🗨**blink** are:
  - Object: arbitrary data defined by a library, each object must explicitly implement the type coercion for all types below. 
  - Unknown: void data. Defaults to 0 for all types, implementation defined by Object
  - Boolean: a flag that represents truthy or falsy values, can be defined by `true y yes on` for truthy values and `false n no off` for falsy values. Note: these are case *insensitive*, meaning 'YES' and 'ofF' are perfectly valid. 
  - Integer: a 32 bit signed integer.
  - Decimal: a 32 bit floating point number.
  - String: a null-terminated UTF-8 string. Can be defined by "double quotation" or 'single quotation'
The type coercion is the strong point of this language, it has the power to convert types intuitively to others. The default conversions are as follows:
  - Unknown: converts to `false`, `0`, `0.0`, `""`. 
  - Boolean: falsy values evaluate to `0`, `0.0`, `"false"`, and truthy to `1`, `1.0`, `"true"`
  - Integer: for Booleans, non zero numbers evaluate to truthy, and zero to falsy. For decimals, the value is expanded. For Strings, the Integer value is stringified.
  - Decimal: for Booleans, values between (-1, 1) (Exclusive) are evaluated as falsy, and the rest as truthy. For Integers, the value is always rounded positively (also known as reverse-truncation), unless the value is 0.001 above the integer threshold, this is to mitigate floating point errors. For strings, the Decimal value is stringified.
  - String: for Booleans, `""`, `"false"`, `"off"`, `"no"`, `"n", "0", "(from -1.0 exclusive to 1.0 exclusive)"` evaluate to falsy values, and everything else to truthy. For integers, strings will always assume the lowest available base first, if only numbers are used, these will be converted to integer using base 10 notation. Else, if alphanumeric characters in the range of `0-9` and `A-F` are used, the String is interpreted as a hexadecimal integer (base 16). Else, if alphanumeric characters in the range of `0-9` and `A-Z` are used, the string is converted following Base 36 notation. If this does not work, the string is assumed to follow Base64 conventions, and will convert each character to it's corresponding number and radix (Note that Base64 is *not* case insensitive). If all else fails, the result will be 0, indicating a conversion failure. Decimals follow a simpler approach, if the number does not evaluate to a Decimal, a `NotANumberException` is thrown. This is to prevent bad values like NaNs plaguing the programme.

### Exceptions
👁‍🗨**blink** has sophisticated exception handling. Exceptions are by default propagated up the coroutine chain. These can be caught by adding square brackets around using the subroutine, and will return an Object that must be resolved using `std.ex.getmsg` to read the exception message, `std.ex.getcod` to read the exception code, `std.ex.unbox[b, i, d, s, o]` to get the underlying value. If an exception was thrown, the value will always return `unknown`.
