// compactor.c

// This file is part of "BoolX"
// Copyright (c) 2021 Andrea Calligaris
// Distributed under the MIT software license, see LICENSE.txt

#include <ctype.h>  //isprint
#include <getopt.h>
#include <stdbool.h>  // bool
#include <stdio.h>    // printf, file stuff
#include <stdlib.h>   //abort

#define LINES_LENGTH_DEFAULT 36

int main(int argc, char *argv[]);
int process_arguments(int argc, char *argv[]);
bool is_instruction_to_be_included(char current_instruction);

bool show_usage = false;
char *source_program_path;
char *output_program_path;
long int lines_length = LINES_LENGTH_DEFAULT;
long long int n_nested_comments = 0;

int main(int argc, char *argv[]) {
  FILE *source_program;
  FILE *output_file;
  char current_instruction;
  bool writing_error = false;
  long int char_line_counter = 0;

  if (process_arguments(argc, argv) != 0) {
    return 1;
  }

  if (show_usage) {
    printf("Usage: compactorx [options] <source_file> <output_file>\n");
    printf(
        "\nA compactor for BoolX code; v1.0."
        "\nRemove comments and other useless characters from <source_file>\n"
        "and save the result to <output_file>, with the goal of creating an\n"
        "artistic and esoteric source code.\n");
    printf(
        "\n  -l N\t\tset the max number of characters in each line to N\n"
        "\t\t(default is %d)\n",
        LINES_LENGTH_DEFAULT);
    return 0;
  }

  source_program = fopen(source_program_path, "r");
  if (source_program == NULL) {
    printf("Can't open the source program file.\n");
    return 1;
  }

  output_file = fopen(output_program_path, "w");
  if (output_file == NULL) {
    printf("Can't open the output file.\n");
    return 1;
  }

  // read the instructions one by one:
  while (fscanf(source_program, "%1c", &current_instruction) == 1) {
    if (current_instruction == '{') {
      n_nested_comments++;
      continue;
    } else if (current_instruction == '}') {
      n_nested_comments--;
      continue;
    }
    if (n_nested_comments > 0) {
      continue;
    }

    if (is_instruction_to_be_included(current_instruction)) {
      if (putc(current_instruction, output_file) == EOF) {
        printf("Error while writing to the output file.\n");
        writing_error = true;
        break;
      }
      printf("%c", current_instruction);
      char_line_counter++;
      if (char_line_counter == lines_length) {
        putc('\n', output_file);
        printf("\n");
        char_line_counter = 0;
      }
    }
  }
    if (writing_error == false) {
      printf("\nDone.\n");
    }

    fclose(output_file);
    fclose(source_program);

    return 0;
}

int process_arguments(int argc, char *argv[]) {
  char *lines_length_arg = NULL;
  int index;
  int c;
  opterr = 0;
  int non_option_argc;

  static struct option long_options[] = {
      {"lines_length", required_argument, NULL, 'l'}, {NULL, 0, NULL, 0}};

  while ((c = getopt_long(argc, argv, "l:", long_options, NULL)) != -1) {
    switch (c) {
      case 'l': {
        lines_length_arg = optarg;
        break;
      }
      case '?': {
        if (optopt == 'l') {
          fprintf(stderr, "Option -%c requires an argument.\n", optopt);
        } else if (isprint(optopt)) {
          fprintf(stderr, "Unknown option `-%c'.\n", optopt);
        } else {
          fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
        }
        return 1;
      }
      default: {
        abort();
      }
    }
  }

  if (lines_length_arg != NULL) {
    lines_length = strtol(lines_length_arg, NULL, 10);
    if (lines_length == 0) {
      fprintf(stderr, "Option '-l' has been given a bad value.\n");
    }
  }

  non_option_argc = argc - optind;

  if (non_option_argc == 0) {
    show_usage = true;
    return 0;
  } else if (non_option_argc == 1) {
    printf("Missing output file.\n");
    return 1;
  } else if (non_option_argc == 2) {
    source_program_path = argv[optind];
    output_program_path = argv[optind + 1];
    return 0;
  } else {
    printf("Too many arguments.\n");
    return 1;
  }
}

bool is_instruction_to_be_included(char current_instruction) {
  switch (current_instruction) {
    case '>': {
      return true;
    }
    case '<': {
      return true;
    }
    case '|': {
      return true;
    }
    case '+': {
      return true;
    }
    case '-': {
      return true;
    }
    case '=': {
      return true;
    }
    case '_': {
      return true;
    }
    case '^': {
      return true;
    }
    case '*': {
      return true;
    }
    case '%': {
      return true;
    }
    case ']': {
      return true;
    }
    case '[': {
      return true;
    }
    case '#': {
      return true;
    }
    case '&': {
      return true;
    }
    case '?': {
      return true;
    }
    case '"': {
      return true;
    }
    case '!': {
      return true;
    }
    case ';': {
      return true;
    }
    case ':': {
      return true;
    }
    case '/': {
      return true;
    }
    case '\\': {
      return true;
    }
    case '$': {
      return true;
    }
    case '\'': {
      return true;
    }
    case '@': {
      return true;
    }
    case '~': {
      return true;
    }
    default: {
      return false;
    }
  }
}
