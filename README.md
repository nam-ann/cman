# cman

An ultralightweight C++ build tool in pure C++23

## Status

Stable

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
### 1. Basic Commands

cman provides a straightforward CLI for cleaning and building your targets.

```bash
# Display help and available build modes
cman help

# Show cman version
cman version

# Clean build artifacts (removes the `out/` directory)
cman clean

# Build a project
cman build [compile_type] [dependencies] [compiler]
```

### 2. Compile Types

When running `cman build`, specify one of the following compilation modes:

| Mode | Target Output | Description |
| :--- | :--- | :--- |
| `exec` | Executable | Builds standalone executable binary (`.exe` / ELF) |
| `dynlnk` | Dynamic Library | Builds dynamic shared library (`.dll` / `.so`) |
| `statlnk` | Static Library | Builds static library archive (`.lib` / `.a`) |

### 3. Configuration Files

`cman` uses simple plain-text configuration files to drive the build pipeline.

#### A. Target Dependencies (`.cdeps`)
Defines the directory layout and source files using indented tree paths and wildcards.

```text
# cmain.cdeps
src/
    *.cpp
module/
    main.cppm
    a.cppm
```

#### B. Compiler Template (`compiler.txt`)
Defines command-line templates for your specific C++ toolchain using indexed placeholders:
* `{0}`: Input source file / object inputs
* `{1}`: Output object file / target binary
* `{2}`: Optional output path for C++ Module interface (BMI/IFC)

**Example for GCC (`gcc_compiler.txt`):**
```text
g++ -std=c++23 -fmodules-ts -c "{0}" -o "{1}" -MMD -MF "{1}.d"
g++ -std=c++23 -fmodules-ts -c "{0}" -o "{1}" -MMD -MF "{1}.d"
g++ -shared {0} -o "{1}.dll" -lstdc++exp -Wl,--allow-multiple-definition
ar rcs "{1}.a" {0}
-I"{0}"
g++ {0} -o "{1}.exe" -lstdc++exp -Wl,--allow-multiple-definition
```

**Example for MSVC (`msvc_compiler.txt`):**
```text
cl /std:c++latest /nologo /EHsc /MT /c "{0}" /Fo"{1}" /sourceDependencies "{1}.d"
cl /std:c++latest /nologo /EHsc /MT /c /TP /interface "{0}" /Fo"{1}" /sourceDependencies "{1}.d"
cl /std:c++latest /nologo /EHsc /MT /LD {0} /Fe"{1}.dll"
lib /nologo /OUT:"{1}.lib" {0}
/I"{0}"
link /nologo /OUT:"{1}.exe" {0}
```

#### C. Root Dependency List (`deps.txt`)
Gathers project `.cdeps` and environment module definitions into a single build entry point.

```text
# gcc_deps.txt
gcc.cdeps
cmain.cdeps
```

### 4. Build Examples

**Building an Executable with GCC:**
```bash
cman build exec gcc_deps.txt gcc_compiler.txt
```

**Building an Executable with MSVC:**
```bash
cman build exec msvc_deps.txt msvc_compiler.txt
```

**Cleaning and Rebuilding in One Line:**
```bash
cman clean && cman build exec deps.txt compiler.txt
```

## Support
If you find this project useful, please consider giving it a star on GitHub. Thank you! :)