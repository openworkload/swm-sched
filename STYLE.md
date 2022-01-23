
C++ CODE STYLE
===============

These guidelines should be followed for all new C++ code in this repository.

The rules are based on some Google style rules.

Reviewers will be enforcing them, so please obey them.


Mandatory rules:
--------------------------

1.  Use .cpp extension instead of .cc
2.  Use "#pragma once" instead of #define guard
3.  No global variables (even const)
4.  Every .cpp file should have an associated .h file [link](https://google.github.io/styleguide/cppguide.html#Header_Files).
5.  Guard all headers [link](https://google.github.io/styleguide/cppguide.html#The__define_Guard).
6.  Includes order: related header -> C system includes (ABS order) -> C++ system includes (ABS order) -> other includes (ABS order) -> project includes (ABS order) -> conditional includes [link](https://google.github.io/styleguide/cppguide.html#Names_and_Order_of_Includes).
7. In constructors no virtual method calls or functions that can fail and if you can't signal an error [link](https://google.github.io/styleguide/cppguide.html#Doing_Work_in_Constructors).
8. No implicit conversions [link](https://google.github.io/styleguide/cppguide.html#Implicit_Conversions).
9. Make class data members private, unless they are static const [link](https://google.github.io/styleguide/cppguide.html#Access_Control)
10. When defining a function, parameter order is: inputs, then outputs [link](https://google.github.io/styleguide/cppguide.html#Function_Parameter_Ordering)
11. All parameters passed to a function by reference must be labeled const [link](https://google.github.io/styleguide/cppguide.html#Reference_Arguments)
12. Use standard smart pointers when ownership can be transferred [link](https://google.github.io/styleguide/cppguide.html#Ownership_and_Smart_Pointers)
13. Use prefix form (++i) of the increment and decrement operators with iterators and other template objects [link](https://google.github.io/styleguide/cppguide.html#Preincrement_and_Predecrement)
14. Use a precise-width integer type from from <stdint.h> for all integer types and size\_t and ptrdiff\_t when required [link](https://google.github.io/styleguide/cppguide.html#Integer_Types)
15. Use 0 for integers, 0.0 for reals, nullptr for pointers, and '\0' for chars [link](https://google.github.io/styleguide/cppguide.html#0_and_nullptr/NULL)
16. Do not use non-standard extensions (exception: #pragma once) [link](https://google.github.io/styleguide/cppguide.html#Nonstandard_Extensions)
17. No namespace aliases [link](https://google.github.io/styleguide/cppguide.html#Aliases)
18. Filenames should be all lowercase and can include underscores (\_) [link](https://google.github.io/styleguide/cppguide.html#File_Names)
19. Type names or enumerators start with a capital letter and have a capital letter for each new word, with no underscores ("Camel Case") [link](https://google.github.io/styleguide/cppguide.html#Type_Names)
20. The names of namespaces, functions/methods, variables (including function parameters), constants and data members are all lowercase, with underscores between words [link](https://google.github.io/styleguide/cppguide.html#Variable_Names)
21. Macros names are uppercase with underscores between the words: THIS\_THIS\_MY\_CONSTANT [link](https://google.github.io/styleguide/cppguide.html#Macro_Names)
22. Each line of text in your code must be at most 120 characters long.
23. Do not use any of non-ASCII characters [link](https://google.github.io/styleguide/cppguide.html#Non-ASCII_Characters)
24. Use spaces, no tabs, and indent 2 spaces at a time [link](https://google.github.io/styleguide/cppguide.html#Spaces_vs._Tabs)
25. Return type on the same line as function name, and its parameters on the same line if they fit [link](https://google.github.io/styleguide/cppguide.html#Function_Declarations_and_Definitions)
26. Always use braces for: switch-case-blocks, while-loop and for-loop bodies; and no empty bodies for loops [link](https://google.github.io/styleguide/cppguide.html#Loops_and_Switch_Statements)
27. No spaces around period or arrow in pointer or reference expression [link](https://google.github.io/styleguide/cppguide.html#Pointer_and_Reference_Expressions)
28. Class sections in public, protected and private order, each indented one space [link](https://google.github.io/styleguide/cppguide.html#Class_Format)
29. The contents of namespaces are not indented [link](https://google.github.io/styleguide/cppguide.html#Namespace_Formatting)

Recomended rules:
----------------------------------------------

31.  Headers should be self-contained [link](//google.github.io/styleguide/cppguide.html#Self_contained_Headers).
32. Use C++-style casts, or brace initialization for conversion of arithmetic types instead of C-like casts [link](https://google.github.io/styleguide/cppguide.html#Casting)
33.  Avoid forward declaration where possible [link](https://google.github.io/styleguide/cppguide.html#Forward_Declarations).
34.  Define inline functions when they are <= 10 lines [link](https://google.github.io/styleguide/cppguide.html#Inline_Functions).
35.  Put the code in namespaces with unique names, but never use using directive [link](https://google.github.io/styleguide/cppguide.html#Namespaces)
36.  Put definitions that will not be used outside .cpp in unnamed namespace or declare them static (do not do that in headers) [link](https://google.github.io/styleguide/cppguide.html#Unnamed_Namespaces_and_Static_Variables).
37.  Place nonmember functions in a namespace [link](https://google.github.io/styleguide/cppguide.html#Nonmember,_Static_Member,_and_Global_Functions).
38. Place a function's variables in the narrowest scope possible, and initialize variables in the declaration [link](https://google.github.io/styleguide/cppguide.html#Local_Variables).
39. Static storage duration variables are allowed only if declared as constexpr and they are POD [link](https://google.github.io/styleguide/cppguide.html#Static_and_Global_Variables).
40. Do not make new classes copyable or movable [link](https://google.github.io/styleguide/cppguide.html#Copyable_Movable_Types)
41. Prefer classes, not structs [link](https://google.github.io/styleguide/cppguide.html#Structs_vs._Classes)
42. Prefer composition on inheritance, but when inherinece is still required, then make it public [link](://google.github.io/styleguide/cppguide.html#Inheritance)
43. No multiple inheritance [link](https://google.github.io/styleguide/cppguide.html#Multiple_Inheritance)
44. Do not overload operators [link](https://google.github.io/styleguide/cppguide.html#Operator_Overloading)
45. In a class group similar declarations together, placing public parts earlier [link](Interface class should ha://google.github.io/styleguide/cppguide.html#Declaration_Order)
46. No functions body longer than 60 lines, excluding comments and empty lines [link](https://google.github.io/styleguide/cppguide.html#Write_Short_Functions)
47. No friend classes [link](https://google.github.io/styleguide/cppguide.html#Friends)
48. Avoid using Run Time Type Information (RTTI) [link](https://google.github.io/styleguide/cppguide.html#Run-Time_Type_Information__RTTI_)
49. Use const or constexpr where possible [link](https://google.github.io/styleguide/cppguide.html#Use_of_const)
50. Code should be 64-bit and 32-bit friendly [link](https://google.github.io/styleguide/cppguide.html#64-bit_Portability)
51. Avoid defining complex macros [link](https://google.github.io/styleguide/cppguide.html#Preprocessor_Macros)
52. Use sizeof(varname) instead of sizeof(type) [link](https://google.github.io/styleguide/cppguide.html#sizeof)
53. Use auto when it increases readability, but do not initialize an auto-typed variable with a braced initializer list [link](https://google.github.io/styleguide/cppguide.html#auto)
54. Avoid overcomplicated template programming [link](https://google.github.io/styleguide/cppguide.html#Template_metaprogramming)
55. Do not define specializations of std::hash [link](https://google.github.io/styleguide/cppguide.html#std_hash)
56. Use C++11/14 features when needed, but no compile-time rational numbers (\<ratio\>), no \<cfenv\> and \<fenv.h\> headers and no ref-qualifiers on member functions [link](https://google.github.io/styleguide/cppguide.html#C++11)
57. No abbreviations in naming, make the names self-descriptive [link](https://google.github.io/styleguide/cppguide.html#General_Naming_Rules)
58. Use either the // or /* */ comment syntax, as long as you are consistent [link] (https://google.github.io/styleguide/cppguide.html#Comment_Style)
59. If a .h declares multiple abstractions, the file-level comment should broadly describe the contents of the file, and how the abstractions are related [link](https://google.github.io/styleguide/cppguide.html#File_Comments)
60. Every class declaration should have an accompanying comment that describes what it is for and how it should be used [link](https://google.github.io/styleguide/cppguide.html#Class_Comments)
61. Use function-wide comments for non-obvious functions [link](https://google.github.io/styleguide/cppguide.html#Function_Comments)
62. No variable (including function arguments) comments, use self-descriptive variable names instead [link](https://google.github.io/styleguide/cppguide.html#Variable_Comments)
63. Add comments for tricky, non-obvious, interesting, or important parts of your code only [link](https://google.github.io/styleguide/cppguide.html#Implementation_Comments)
64. Pay attention to punctuation, spelling, and grammar in the comments [link](https://google.github.io/styleguide/cppguide.html#Punctuation,_Spelling_and_Grammar)
65. Use TODO or FIXME comments for code that is temporary, a short-term solution [link](https://google.github.io/styleguide/cppguide.html#TODO_Comments)
66. Each line of text in your code should be less then 80 characters long [link](https://google.github.io/styleguide/cppguide.html#Line_Length)
67. Format parameters and bodies of lambdas as for any other function, and capture lists like other comma-separated lists [link](https://google.github.io/styleguide/cppguide.html#Formatting_Lambda_Expressions)
68. Format a braced initializer list exactly like you would format a function call in its place [link](https://google.github.io/styleguide/cppguide.html#Braced_Initializer_List_Format)
69. No spaces inside parentheses with one condition, one space after if, one space after closing paranthes, and put the if and else keywords on separate lines [link](https://google.github.io/styleguide/cppguide.html#Conditionals)
70. No multiple variables in one declaration if those have pointer or reference decorations [link](https://google.github.io/styleguide/cppguide.html#Pointer_and_Reference_Expressions)
71. When break long boolean expressions then put boolean operator at the end of the line [link](https://google.github.io/styleguide/cppguide.html#Boolean_Expressions)
72. Never put trailing whitespaces at the end of a line [link](https://google.github.io/styleguide/cppguide.html#Horizontal_Whitespace)
73. Do not use C++ exceptions is possible [link](https://google.github.io/styleguide/cppguide.html#Exceptions)


In all other cases use common sense and **be consistent**.
