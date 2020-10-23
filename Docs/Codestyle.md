CPP Codestyle
========

## General

#### Spaces vs. Tabs
Use oly spaces, 4 spaces for indent.
Do not use tabs in your code. 

#### Line Length
There is no limitation for line of text in your code. 
Try to meet your code 120 characters per line. This will help to easy look at your code in side-by-side mode. 

## Header Files

#### Header guard
Header files are guarder by `#pragma once`

#### Includes and Order of Includes
When listing includes, order them by increasing generality: your project's `.h`, other modules `.h`, other libraries `.h`, C++ library, C library.

Your project `.h` must be included with `#include "Your/Project/Path.h"` while external `.h` should be included with `#include <Library/Path.h>`

```cpp
//your project
#include "Your/Project/Path.h"

//common modules
#include <TArc/Core/ClientModule.h>

//DAVA library
#include <Base/BaseTypes.h>

//stl
#include <vector>
```

Within each section the includes should be ordered alphabetically. Note that older code might not conform to this rules and should be fixed when convenient.

```cpp
//common modules
#include <TArc/Core/ClientModule.h>
#include <TArc/Core/ContextAccessor.h>
#include <TArc/DataProcessing/DataContext.h>
```

All of a project's header files should be listed as descendants of the project's source directory without use of UNIX directory shortcuts `.` (the current directory) or `..` (the parent directory). For example, ../BaseTypes.h should be included as:

```cpp
// for Base/BaseTypes.cpp file
#include "Base/BaseTypes.h"
```

## Scoping

#### Namespaces
Intendation is not used inside namespaces

##### namespace DAVA
All DAVA Engine code (including modules, editors) is written inside `namespace DAVA`.

```cpp
namespace DAVA
{
class Foo
{
    void Bar();
};
}
```


##### Nested namespaces
Do not use nested namespaces, i.e. `namespace MyModule`. Just work inside `namespace DAVA`. Use class name prefixes instead of nested namespaces to avoid name collisions.
For example: `namespace DAVA{ class RenderObject; }` instead of `namespace DAVA { namespace Render { class Object; } }`.
The only exceptions are local(private) namespaces.

###### Local namespaces
Local(private) namespaces (are used to hide implementation details) must use `Details` suffix, i.e. `MyUtilsDetails`. Local(private) namespaces are used instead of unnamed namespaces to avoid collisions in unity builds.

#### Commented code
Remove commented code, we have revision control system for history. 

## Naming
Names are written in english, with no prefixes/suffixes and no odd shortenings. Name length and descriptiveness should be proportional to name scope: more descriptive names are used in bigger scopes.

```cpp
Image * CreateImageFromMemory(...); //long descriptive function name in public interface
```

```cpp
{
    void * imageData = image->Data(); //imageData is local variable name
    //plenty of code here
}
```

Note that certain universally-known abbreviations are OK, such as 'i' for an iteration variable and 'T' for a template parameter.

```cpp
for(int32 i = 0; i < width; ++i) //i is only used inside small loop
{
    Read(i);
}
```

```cpp
template<typename T>
void GetTypeInstance() { ... }
```

Don't use upper-case abbreviations.

```cpp
class DeviceImplMacOs;  // NOT DeviceImplMacOS
class NetworkId;        // NOT NetworkID
class Engine
{
    uint32 maxFps;      // NOT maxFPS
};
```

#### Classes, functions
Camel notation begin with capital letter.

```cpp
class MyObject;
void Foo();
```

#### Utils classes
Additional functionality for specified class should be placed in separate files with class with suffix 'Utils'. 

```cpp
//Profiler.h
class Profiler{...}

//ProfilerUtils.h
class ProfilerUtils{...}
```

#### Variables
Camel notation begin with lower-case.

```cpp
MyObject myObjectInstance;
```
In case of names collision add underscore to local variable name.

```cpp
MyObject::MyObject(const String& name_)
:   name(name_)
{...}
```

#### Static constants, enums
Variables declared constexpr or const should be named with camel notation beginnig with upper-case. 

```cpp
static const int MaxWidth = 100;
struct Date
{
    const int DaysInWeek = 7;
};
```

The individual enumerators should be named like constants.
```cpp
enum Example 
{
    Left = 0,
    Right,
    LeftOrRight
};
```

#### Function objects
Function object is both function and variable. We just use the same naming rules as for variables: camel notation begin with lower-case.

```cpp
auto foo()[]{}
```

#### Macro
All capitals with underscores.

```cpp
#ifdef DAVAENGINE
#define HAS_SPECULAR
#endif
```

## Types
Use types from `Base\BaseTypes.h` instead of built-in.
This include

* base types (int32, float32, etc.)
* standart library types (String, Vector, etc.)

## Classes
#### Class members initialization
In-class member initialization is preferred over all other methods.

```cpp
class MyObject
{
    int32 width = 0;
    int32 height = 0;
    Color color = Color::White();
};
```

#### Virtual functions
Either `virtual`, `override` or `final` keyword can be used in function declaration. **Do not** mix `virtual` with `override/final` keywords in one declaration.

```cpp
virtual void Foo(); //declared for the first time
void Bar() override; //overriding virtual function
```

#### Inline functions
Separate inline function definition from declaration and use `inline` keyword only in declaration. Declaration should be placed in the end of header file.

```cpp
class MyObject
{
    void Foo();
};
//...
inline void MyObject::Foo()
{
}
```
## CPP 11+ features
#### Autos
Usage of auto is limited to
##### lambda function type

```cpp
auto foo = [&](bool arg1, void* arg2)
{
    arg1 = *arg2;
}
```
and
##### shortening of long template types

```cpp
Map<FastName, SmartPointer<ObjectType>> map;
auto iter = map.begin();
```

## Comments

Remember: comments are very important, but the best code is self-documenting. Giving sensible names to types and variables is much better than using obscure names that you must then explain through comments.

#### Comment Style
We use doxygen commenting-style for public definitions (e.g. `.h` files) with some with custom exceptions:
 - All comment should be written in "Desing by Contract" style. See [en.wiki](https://en.wikipedia.org/wiki/Design_by_contract) or [ru.wiki](https://ru.wikipedia.org/wiki/%D0%9A%D0%BE%D0%BD%D1%82%D1%80%D0%B0%D0%BA%D1%82%D0%BD%D0%BE%D0%B5_%D0%BF%D1%80%D0%BE%D0%B3%D1%80%D0%B0%D0%BC%D0%BC%D0%B8%D1%80%D0%BE%D0%B2%D0%B0%D0%BD%D0%B8%D0%B5) for more info.
 - Use `/** */` syntax for overal public class or function description.
 - Use `//!` syntax for public variable or enum description.
 - Avoid `\brief`, `\param` or `\return` doxygen keyworlds.

Note that `/** */` syntax has single-line or multi-line style:
```cpp
/** Single line comment **/
void foo(); 

/**
Multiple 
lines
comment
*/
void boo();
```

Also you can use either the `//` or the `/* */` syntax in private implementation (e.g. `.cpp` or `Private/.h` files). 

Please, be consistent with how you comment and what style you use where.

#### File Comments
- We don't use any licence headers.
- Public `.h` files have to be well commented.
- Do not duplicate comments in both the `.h` and the `.cpp`.

#### Class Comments
Every public class declaration should have an accompanying comment that describes what it is for and how it should be used. 

The class comment should provide the reader with enough information to know how and when to use the class, as well as any additional considerations necessary to correctly use the class. 

The class comment is often a good place for a small example code snippet demonstrating a simple and focused usage of the class.

```cpp
/** 
    The class Any is a type-safe container for single value of any type.
    Stored value is always copied into internal storage. Implementations is encouraged to 
    avoid dynamic allocations for small objects, but such an optimization may only
    be applied to types for which std::is_nothrow_move_constructible returns true.

    Any can be copied.
    - Internal storage with trivial value will also be copied.
    - Internal storage with complex type will be shared the same way as `std::shared_ptr` do.

    Typical usage:
    void foo()
    {
        Any a;
        a.Set(int(1));
        std::count << a.Get<int>(); // prints "1"
    }
*/
class Any final
{
    // ...
};
```

When class is separated on inteface definition and implementation (e.g. `.h` and `.cpp` files), comments describing the use of the class should go together with its interface definition; comments about the class operation and implementation should be inside class's methods implementation.



#### Function Comments
Declaration comments describe use of the function (when it is non-obvious); comments at the definition of a function describe operation.

## Unittests
Unittests files should be named 'MyClass.unittest.cpp' and placed along with .cpp files with tested code.

```
Profiler.cpp
Profiler.unittest.cpp
```

Python codestyle
========
Follow PEP8: https://www.python.org/dev/peps/pep-0008/
