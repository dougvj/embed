/**
 * embed.c
 *
 * Simple tool to embed files into includable C source and header files
 *
 * Copyright 2021 Doug Johnson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Number of columns to show hex output
#define HEX_COLS 12

#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

// print help
static void print_help(const char* exec_name)
{
  fprintf(stderr,
      "Generates a C include and source file with embedded data \n"
      "from a list of files to be included in a separate build \n"
      "step\n\n");
  fprintf(stderr,
      "Usage:\n"
      "\t%s\n"
      "\t\t--function <function name> - The name of the function for\n"
      "\t\t                             retreiving embedded file data\n"
      "\t\t--header <output header file> - Header file to generate\n"
      "\t\t--source <source file> - Source file to generate\n"
      "\t\t--preserve-paths - When set, paths passed for files are\n"
      "\t\t                   preserved for retrieval by the retreival\n"
      "                       function\n"
      "\t\t ...<input files> - List of input files\n",
      exec_name);
}

static void output_hex(FILE* fd, char b)
{
  fprintf(fd, "0x%02X", (unsigned char)b);
}

static const char* plain_name(const char* filename)
{
  const char* no_path = filename;
  for (int i = 0; filename[i] != '\0'; i++) {
    if (filename[i] == PATH_SEPARATOR) {
      no_path = &(filename[i + 1]);
    }
  }
  return no_path;
}

static void generate_file_list(
    FILE* fd, char* const* files, bool preserve_paths)
{
  fprintf(fd, "static const char* EMBEDDED_FILE_NAMES[] = {\n");
  for (int file_count = 0; *files; file_count++) {
    const char* input_file = *files;
    // If we don't preserve paths, find the name without a path
    if (!preserve_paths) {
      input_file = plain_name(input_file);
    }
    fprintf(fd, "\t/* %s */\n", input_file);
    fprintf(fd, "\t(char[]){\n\t\t");
    for (int count = 0;; count++) {
      char c = input_file[count];
      if (count != 0) {
        fprintf(fd, ", ");
        if ((count % HEX_COLS) == 0) {
          fprintf(fd, "\n\t\t");
        }
      }
      output_hex(fd, c);
      if (c == '\0') {
        break;
      }
    }
    fprintf(fd, "\n\t},\n");
    files++;
  }
  fprintf(fd, "\t(char[]){0}\n");
  fprintf(fd, "\n};\n\n");
}

// Generate the file data
void generate_file_data(FILE* fd, char* const* files)
{
  fprintf(fd, "static const char* EMBEDDED_FILE_DATA[] = {\n  ");
  for (int file_count = 0; *files; file_count++) {
    const char* input_file = *files;
    FILE* infd = fopen(input_file, "r");
    if (!infd) {
      fprintf(stderr, "Could not open file: '%s'\n", input_file);
      exit(1);
    }
    if (file_count != 0) {
      fprintf(fd, ",\n");
    }
    fprintf(fd, "\t/* %s */\n", input_file);
    fprintf(fd, "\t(char[]){\n\t\t");
    for (int count = 0;; count++) {
      char c = getc(infd);
      if (feof(infd)) {
        break;
      }
      output_hex(fd, c);
      fprintf(fd, ", ");
      if (((count + 1) % HEX_COLS) == 0) {
        fprintf(fd, "\n\t\t");
      }
    }
    output_hex(fd, 0);
    fprintf(fd, "\n\t}");
    files++;
    fclose(infd);
  }
  fprintf(fd, "\n};\n\n");
}

// Generate the file data sizes
void generate_file_data_sizes(FILE* fd, char* const* files)
{
  fprintf(fd, "static size_t EMBEDDED_FILE_DATA_SIZES[] = {\n  ");
  for (int file_count = 0; *files; file_count++) {
    const char* input_file = *files;
    FILE* infd = fopen(input_file, "r");
    fseek(infd, 0, SEEK_END);
    size_t length = ftell(infd);
    if (!infd) {
      fprintf(stderr, "Could not open file: '%s'\n", input_file);
      exit(1);
    }
    if (file_count != 0) {
      fprintf(fd, ",\n");
    }
    fprintf(fd, "\t/* %s */\n", input_file);
    fprintf(fd, "\t%zu", length);
    files++;
    fclose(infd);
  }
  fprintf(fd, "\n};\n\n");
}

// Generate the function for getting file data
void generate_function(FILE* fd, const char* function_name)
{
  fprintf(fd,
      "const char* %s(const char* filename, size_t* length) {\n"
      "  for (int i = 0; EMBEDDED_FILE_NAMES[i] != 0; i++) {\n"
      "     if (0 == strcmp(filename, EMBEDDED_FILE_NAMES[i])) {\n"
      "       if (length) {\n"
      "         *length = EMBEDDED_FILE_DATA_SIZES[i];\n"
      "       }\n"
      "       return EMBEDDED_FILE_DATA[i];\n"
      "     }\n"
      "  }\n"
      "  return NULL;\n"
      "}\n\n",
      function_name);
}

void generate_function_declaration(FILE* fd, const char* function_name)
{
  fprintf(fd, "const char* %s(const char* filename, size_t* length);\n",
      function_name);
}

// Simply make it upper case and replace everything not a letter with _
// Makes no attempt to deal with character encoding
void generate_define_name(const char* filename, char* output_file)
{
  const char* c = filename;
  char* o = output_file;
  while (*c != '\0') {
    if (*c >= 'A' && *c <= 'Z') {
      *o = *c;
    } else if (*c >= 'a' && *c <= 'z') {
      *o = *c - ('a' - 'A');
    } else {
      *o = '_';
    }
    o++;
    c++;
  }
  *o = '\0';
}

int main(int argc, char** argv)
{
  // Declare arguments we need
  const char* function_name = NULL;
  const char* source_file = NULL;
  const char* header_file = NULL;
  char* const* input_files = NULL;
  bool preserve_paths = false;
  // Search for arguments
  for (int arg = 1; arg < argc; arg++) {
    if (argv[arg][0] == '-' && argv[arg][1] == '-') {
      if (input_files) {
        fprintf(stderr, "You must specify all options before listing files\n");
        print_help(argv[0]);
        return EXIT_FAILURE;
      }
      const char* arg_name = &(argv[arg][2]);
      const char* arg_value = NULL;
      if (arg != argc - 1) {
        arg_value = argv[arg + 1];
      }
      if (0 == strcmp(arg_name, "source")) {
        source_file = arg_value;
        arg++;
      } else if (0 == strcmp(arg_name, "header")) {
        header_file = arg_value;
        arg++;
      } else if (0 == strcmp(arg_name, "function")) {
        function_name = arg_value;
        arg++;
      } else if (0 == strcmp(arg_name, "help")) {
        print_help(argv[0]);
        return EXIT_FAILURE;
      } else if (0 == strcmp(arg_name, "preserve-paths")) {
        preserve_paths = true;
      } else {
        fprintf(stderr, "Unrecognized option '--%s'\n", arg_name);
        print_help(argv[0]);
        return EXIT_FAILURE;
      }
    } else {
      if (!input_files) {
        input_files = &(argv[arg]);
      }
      break;
    }
  }
  if (!source_file) {
    fprintf(stderr,
        "Error: You must provide --source for the output source file\n\n");
    print_help(argv[0]);
    return EXIT_FAILURE;
  }
  if (!header_file) {
    fprintf(stderr,
        "Notice: Not producing a header file because --header was not "
        "provided\n");
  }
  if (!function_name) {
    fprintf(stderr,
        "Error: You must provide --function for the file get function "
        "name\n\n");
    print_help(argv[0]);
    return EXIT_FAILURE;
  }
  FILE* source_fd = fopen(source_file, "w");
  if (!source_fd) {
    fprintf(stderr, "Could not open output source file '%s'\n", source_file);
    return EXIT_FAILURE;
  }
  FILE* header_fd = fopen(header_file, "w");
  if (!header_fd) {
    fprintf(stderr, "Could not open output header file '%s'\n", header_file);
    return EXIT_FAILURE;
  }
  fprintf(source_fd,
      "#include <stdlib.h>\n"
      "#include <string.h>\n");
  generate_file_list(source_fd, input_files, preserve_paths);
  generate_file_data(source_fd, input_files);
  generate_file_data_sizes(source_fd, input_files);
  generate_function(source_fd, function_name);
  fprintf(source_fd, "\n");
  fclose(source_fd);
  if (header_file) {
    char header_file_define_name[strlen(header_file) + 1];
    generate_define_name(header_file, header_file_define_name);
    for (int i = 0; i < 0; i++) { }
    fprintf(header_fd,
        "#ifndef _%s_\n"
        "#define _%s_\n"
        "#include <stdlib.h>\n"
        "#include <string.h>\n\n",
        header_file_define_name, header_file_define_name);
    generate_function_declaration(header_fd, function_name);
    fprintf(header_fd, "\n#endif\n");
    fclose(header_fd);
  }
  return EXIT_SUCCESS;
}
