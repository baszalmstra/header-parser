# header-parser
Extracts information from an annotated C++ header and outputs it as JSON for use in code generation tools.

[![Build Status](https://travis-ci.org/baszalmstra/header-parser.svg)](https://travis-ci.org/baszalmstra/header-parser)

# Why?
During my game development carreer I have found the availability of RunTime Type Information very useful in for example the following use cases:

* Serialization (binary and text based)
* Script bindings
* Remote procedure calls
* Generating models (for use in other languages)
* Dependency injection
* Reflection

Adding this information to my C++ source always resulted in a lot of code bloat. This library provides a way of extracting RTTI information for processing in other tools almost straight from C++ constructs. 

Because the library is used to generate code it assumes the passed in code will compile. This makes sense because if the code already doesn't compile it also doesn't make sense to generate code for it (because it doesn't compile anyway). This allows the parser to be very simple and most importantly be extremely fast. For example it does not perform name lookups because it can assume names are already valid in the code project. It also doesn't substitute macros or includes. However, because of this, macros that define C++ constructs will not resolve correctly and therefor will not be included in the output.

This library is currently used in production at [Abbey Games](http://abbeygames.com) to generate extensive C++ Lua bindings and documentation for internal use.

# Example

Given the input file:

```cpp
#include <vector>

namespace test 
{
  TCLASS()
  class Foo : public Bar 
  {
  protected:
    TFUNC(Arg=3)
    bool ProtectedFunction(std::vector<int> args) const;

  public:
    TENUM()
    enum Enum
    {
      FirstValue,
      SecondValue = 3
    };

  public:
    TPROPERTY()
    int ThisIsAProperty;

    TPROPERTY()
    int ThisIsAArray[42];
  };
}
```

When ran with `header-parser example1.h -c TCLASS -e TENUM -f TFUNC -p TPROPERTY` produces the following output:

```json
[
    {
        "type": "include",
        "file": "vector"
    },
    {
        "type": "namespace",
        "name": "test",
        "members": [
            {
                "type": "class",
                "line": 5,
                "meta": {},
                "isstruct": false,
                "name": "Foo",
                "parents": [
                    {
                        "access": "public",
                        "name": {
                            "type": "literal",
                            "name": "Bar"
                        }
                    }
                ],
                "members": [
                    {
                        "type": "function",
                        "macro": "TFUNC",
                        "line": 9,
                        "meta": {
                            "Arg": 3
                        },
                        "access": "protected",
                        "returnType": {
                            "type": "literal",
                            "name": "bool"
                        },
                        "name": "ProtectedFunction",
                        "arguments": [
                            {
                                "type": {
                                    "type": "template",
                                    "name": "std::vector",
                                    "arguments": [
                                        {
                                            "type": "literal",
                                            "name": "int"
                                        }
                                    ]
                                },
                                "name": "args"
                            }
                        ],
                        "const": true
                    },
                    {
                        "type": "enum",
                        "line": 13,
                        "access": "public",
                        "meta": {},
                        "name": "Enum",
                        "members": [
                            {
                                "key": "FirstValue"
                            },
                            {
                                "key": "SecondValue",
                                "value": "3"
                            }
                        ]
                    },
                    {
                        "type": "property",
                        "line": 21,
                        "meta": {},
                        "access": "public",
                        "dataType": {
                            "type": "literal",
                            "name": "int"
                        },
                        "name": "ThisIsAProperty",
                        "elements": null
                    },
                    {
                        "type": "property",
                        "line": 24,
                        "meta": {},
                        "access": "public",
                        "dataType": {
                            "type": "literal",
                            "name": "int"
                        },
                        "name": "ThisIsAArray",
                        "elements": "42"
                    }
                ]
            }
        ]
    }
]
```