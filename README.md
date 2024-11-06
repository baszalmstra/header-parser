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
		TPROPERTY()
		int ThisIsAPrivateProperty;
	protected:
		TFUNC(Arg=3)
		bool ProtectedFunction(std::vector<int> args) const;

	public:
		TCONSTRUCTOR()
		Foo() = default;
		TCONSTRUCTOR(Arg=5)
		Foo(int value);

    TENUM()
      enum Enum
    {
      FirstValue,
      SecondValue = 3
    };

	public:
		TPROPERTY()
		int ThisIsAProperty;
  };



	TCLASS()
	template<typename T, typename Base=Foo>
	class TemplatedFoo : public Base
  {

	};
}

TCLASS()
struct Test {
	TPROPERTY()
	int ThisIsAPublicArray1[1];
	
	TPROPERTY()
	int ThisIsAPublicArray2[CONST];

	TPROPERTY()
	int ThisIsAPublicProperty;
private:
	TPROPERTY()
	int ThisIsAPrivateProperty;
};
```

When ran with `header-parser example.h -c TCLASS -e TENUM -f TFUNC -p TPROPERTY -q TCONSTRUCTOR` produces the following output:

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
                "line": 7,
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
                        "type": "property",
                        "line": 10,
                        "meta": {},
                        "access": "private",
                        "dataType": {
                            "type": "literal",
                            "name": "int"
                        },
                        "name": "ThisIsAPrivateProperty",
                        "elements": null
                    },
                    {
                        "type": "function",
                        "macro": "TFUNC",
                        "line": 13,
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
                        "type": "constructor",
                        "line": 17,
                        "meta": {},
                        "access": "public",
                        "name": "Foo",
                        "arguments": [],
                        "default": true
                    },
                    {
                        "type": "constructor",
                        "line": 19,
                        "meta": {
                            "Arg": 5
                        },
                        "access": "public",
                        "name": "Foo",
                        "arguments": [
                            {
                                "type": {
                                    "type": "literal",
                                    "name": "int"
                                },
                                "name": "value"
                            }
                        ]
                    },
                    {
                        "type": "enum",
                        "line": 22,
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
                        "line": 30,
                        "meta": {},
                        "access": "public",
                        "dataType": {
                            "type": "literal",
                            "name": "int"
                        },
                        "name": "ThisIsAProperty",
                        "elements": null
                    }
                ]
            },
            {
                "type": "class",
                "line": 36,
                "meta": {},
                "template": {
                    "arguments": [
                        {
                            "typeParameterKey": "typename",
                            "name": "T"
                        },
                        {
                            "typeParameterKey": "typename",
                            "name": "Base",
                            "defaultType": {
                                "type": "literal",
                                "name": "Foo"
                            }
                        }
                    ]
                },
                "isstruct": false,
                "name": "TemplatedFoo",
                "parents": [
                    {
                        "access": "public",
                        "name": {
                            "type": "literal",
                            "name": "Base"
                        }
                    }
                ],
                "members": []
            }
        ]
    },
    {
        "type": "class",
        "line": 44,
        "meta": {},
        "isstruct": true,
        "name": "Test",
        "members": [
            {
                "type": "property",
                "line": 46,
                "meta": {},
                "access": "public",
                "dataType": {
                    "type": "literal",
                    "name": "int"
                },
                "name": "ThisIsAPublicArray1",
                "elements": "1"
            },
            {
                "type": "property",
                "line": 49,
                "meta": {},
                "access": "public",
                "dataType": {
                    "type": "literal",
                    "name": "int"
                },
                "name": "ThisIsAPublicArray2",
                "elements": "CONST"
            },
            {
                "type": "property",
                "line": 52,
                "meta": {},
                "access": "public",
                "dataType": {
                    "type": "literal",
                    "name": "int"
                },
                "name": "ThisIsAPublicProperty",
                "elements": null
            },
            {
                "type": "property",
                "line": 55,
                "meta": {},
                "access": "private",
                "dataType": {
                    "type": "literal",
                    "name": "int"
                },
                "name": "ThisIsAPrivateProperty",
                "elements": null
            }
        ]
    }
]
```