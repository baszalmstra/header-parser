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

I plan to create several tools using this library to do the above.

# Roadmap
- [x] Create a simple tokenizer
- [x] Use the tokenizer to find attributes
- [x] Extract information from the annotations
- [ ] Extract the source contents starting from the attribute
  - [x] Enums
  - [x] Namespaces
  - [x] Classes
  - [x] Functions
  - [ ] Member variables
- [ ] Enable custom macro extraction

## Optionally
- [ ] Translate raw json to a more consumable format by 3rd party tools
