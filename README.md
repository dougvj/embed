# Embed any set of files in C

Man, it would be nice if we didn't have to bundle so many files with the
executable

## Usage

Build `embed.c` with any C compiler:

```bash
cc embed.c -o embed
```

Also achievable with `make`.

Invoke with 3 arguments, `--source` `--header` and `--function` for the source
file name, generated header name, and function name for getting the data
respectively, then a list of files to embed:

```bash
./embed --source shader_sources.c --header shader_sources.h --function get_shader_source shader_foo.glsl shader_bar.glsl
```

You can then compile and include `shader_sources.c` and `shader_sources.h` in
your program. Getting a file data is as simple as:

```c
#include "shader_sources.h"
size_t size;
const char* data = get_shader_source("shader_foo.glsl", &size);
```

All embedded data is also null terminated. If you know that a file is text and
contains no null bytes, you may simply pull the data as a null terminated
string:

```c
const char* data = get_shader_source("shader_foo.glsl", NULL);
```

If you pass a file with a path, such as `shaders/foo.glsl` the file is
retrievable by the plain file name without a path, `foo.glsl`. You can disable
this behavior by passing the `--preserve-paths` option

The tool is designed to be invokable multiple times to embed sets of files
grouped logically. For example, in addition to embedding shaders one could also
embed textures in a separate pass with a function name `get_texture_data`

# Using with build tools

## Meson

Meson makes it really easy to use `embed` in your build process.

First, add embed as a subproject in your git repository:

`git submodule add http://github.com/dougvj/embed subprojects/embed`


Then in your meson.build, declare the subproject and pull the embed executable

```meson
embed = subproject('embed').get_variable('exe')
```

Then create a custom target to generate the files

```meson
embedded_files = custom_target(
  output: ['shader_data.c', 'shader_data.h']
  input: ['shader_foo.glsl', 'shader_bar.glsl']
  command: [embed,
            '--function', 'get_shader_source',
            '--source', '@OUTPUT0@',
            '--header', '@OUTPUT1@',
            '@INPUT@'])
```

Then include `embedded_files` in your list of sources for the final executable:

```
executable('some_final_executable', [embedded_files] + sources, dependencies)
```

## CMake, Make, etc

Totally doable. Pull requests for examples, or anything else, are welcome

# Why not just use `ld` or `xdd` to embed binary data?

Because writing my own tools from scratch is its own reward ;)

