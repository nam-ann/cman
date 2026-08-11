# cman

cman (C++ Manager) - An ultralightweight C++ build tool written in pure C++23.

## License

MIT License

>  Copyright (c) 2026 nam-ann
>
>  Permission is hereby granted, free of charge, to any person obtaining a copy
>  of this software and associated documentation files (the "Software"), to deal
>  in the Software without restriction, including without limitation the rights
>  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
>  copies of the Software, and to permit persons to whom the Software is
>  furnished to do so, subject to the following conditions:
>
>  The above copyright notice and this permission notice shall be included in
>  all copies or substantial portions of the Software.
>
>  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
>  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
>  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
>  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
>  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
>  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
>  THE SOFTWARE.

## Usage
### 1. Build

Nah bro it's only one cpp file, don't tell me you don't even know how to compile.

**Example for Windows:**
```bash
g++ -std=c++23 -fmodules-ts main.cpp -static -o cman.exe
```

**Example for Linux-based OS:**
```bash
g++ -std=c++23 -fmodules-ts main.cpp -static -o cman
```

Or using MSVC(cl) or Clang (clang++) idc

### 2. Basic Commands

cman provides a straightforward CLI for cleaning and building your targets.

```bash
# Display help and available build modes
cman help

# Show cman version
cman version

# Clean build artifacts (removes the out/ directory)
cman clean

# Build a project
cman build [compile_type] [dependencies] [compiler]
```

### 3. Compile Types

When running `cman build`, specify one of the following compilation modes:

| Mode | Target Output | Description |
| :--- | :--- | :--- |
| `exec` | Executable | Builds standalone executable binary (`.exe` / ELF) |
| `dynlnk` | Dynamic Library | Builds dynamic shared library (`.dll` / `.so`) |
| `statlnk` | Static Library | Builds static library archive (`.lib` / `.a`) |

### 4. Configuration Files

`cman` uses simple plain-text configuration files to drive the build pipeline.

#### A. Target Dependencies (`project.cdeps`)
Defines the directory layout and source files using indented tree paths and wildcards.

```text
src/
    *.cpp
module/
    main.cppm
lib/
    some/
        include/
            .inc
        bin/
            some.lib
            some.a
./
    other.cdeps
```

#### B. Compiler Template (`compiler.txt`)
Defines command-line templates for your specific C++ toolchain using indexed placeholders:
* `{0}`: Input source file / object inputs
* `{1}`: Output object file / target binary

And lines
* line 1: Build Source
* line 2: Build Module
* line 3: Export dynamic link
* line 4: Export static link
* line 5: Add include directory
* line 6: Export executable

**Example for GCC:**
```text
g++ -std=c++23 -fmodules-ts -c "{0}" -o "{1}" -MMD -MF "{1}.d"
g++ -std=c++23 -fmodules-ts -c "{0}" -o "{1}" -MMD -MF "{1}.d"
g++ -shared {0} -o OUT.so -lstdc++exp -Wl,--allow-multiple-definition
ar rcs OUT.a {0}
-I "{0}"
g++ {0} -o OUT -lstdc++exp -Wl,--allow-multiple-definition
```

**Example for MSVC:**
```text
cl /std:c++latest /nologo /EHsc /MT /c "{0}" /Fo"{1}" /sourceDependencies "{1}.d"
cl /std:c++latest /nologo /EHsc /MT /c /TP /interface "{0}" /Fo"{1}" /sourceDependencies "{1}.d"
cl /std:c++latest /nologo /EHsc /MT /LD {0} /Fe OUT.dll
lib /nologo /OUT:OUT.lib {0}
/I "{0}"
link /nologo /OUT:OUT.exe {0}
```

### 5. Build Examples

```bash
cman build exec project.cdeps compiler.txt
```

## Support
If you find this project useful, please consider giving it a star on GitHub. Thank you! :)