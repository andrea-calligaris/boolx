/*
 * interpreter.c
 *
 * This file is part of "BoolX"
 * Copyright (c) 2021 Andrea Calligaris
 * Distributed under the MIT License, see "license.txt"
 */

#include <ctype.h> //isprint
#include <getopt.h>
#include <math.h>    // pow
#include <stdbool.h> // bool
#include <stdio.h>   // printf, getchar, file stuff
#include <stdlib.h>  // malloc
#include <stdlib.h>  // abort
#include <string.h>  // string stuff

#define BOOL1_T bool // See comment for "ignored_starting_bit".

struct TDebugState;

struct TBit;
struct TCell;
struct TGlobalCell;
struct TLabel;
struct TIfElseStatement;

static bool my_strcpy(char *destination, char *source, size_t max_length);

static int process_arguments(int argc, char *argv[]);

static void register_all_labels(FILE *source_program);
static void
execute_source_program_function(FILE *source_program, long long int from_pos);
static void process_current_instruction(FILE *source_program);

static void instruction_if_condition_common();
static void instruction_if_condition_equal_to_1();
static void instruction_if_condition_equal_to_null();
static void instruction_else_condition();
static void instruction_end_of_if_else_statement();
static void instruction_go_to_next_cell();
static void instruction_go_to_previous_cell();
static void instruction_go_to_next_bit();
static void instruction_go_to_previous_bit();
static void instruction_go_to_first_cell();
static void instruction_go_to_first_bit();
static void instruction_set_bit_to_zero();
static void instruction_set_bit_to_one();
static void instruction_set_bit_to_null();
static void instruction_set_all_bits_to_null_and_go_to_first_bit();
static void instruction_print_cell_value_as_ASCII_character();
static void instruction_get_ASCII_input_and_save_as_cell_value();
static void instruction_global_queue_enqueue();
static void instruction_global_queue_dequeue();
static void instruction_select_next_label();
static void instruction_select_previous_label();
static void instruction_select_first_label();
static void instruction_jump_to_label(FILE *source_program);
static void instruction_call_function();
static void instruction_return();

static void dbg_print_labels();
static void dbg_print_instruction();
static void dbg_print_stack_info();
static void dbg_print_n_cells(int n);
static void dbg_print_n_global_cells(int n);
static void dbg_print_cell_value_common(
    struct TBit *ignored_starting_bit, struct TBit *selected_bit);
static void dbg_print_cell_value(struct TCell *cell);
static void dbg_print_global_cell_value(struct TGlobalCell *gl_cell);

static void clear_if_else_statements();
static void free_local_function_memory();
static void free_global_variables();
static void free_cell_content(struct TCell *cell);
static void free_global_cell_content(struct TGlobalCell *g_cell);

static void output(struct TCell *cell);
static void input(struct TCell *cell);

static void process_errors();

enum condition_types { CONDITION_IF, CONDITION_ELSE };
enum errors {
	OK = 0,
	ERR_STRING_TOO_LONG,
	ERR_MISPLACED_ELSE,
	ERR_END_IF,
	ERR_LABEL_CURSOR_OUTSIDE_OF_BOUNDS,
	ERR_JUMP_BUT_NO_LABEL,
	ERR_SEEK_PROGRAM_POSITION,
	ERR_EMPTY_GLOBAL_STACK,
	ERR_USER_INPUT,
};

static const bool PRINT_NEW_LINE_AFTER_TERMINATION = true;
static const bool DBG_SKIP_COMMENTS = true;
static const bool DBG_SKIP_NON_EXECUTED_INSTRUCTIONS = true;
static const bool DBG_SKIP_EMPTY_CHARACTERS = true;
static const bool DBG_ONLY_PRINT_BITS_IN_READABLE_ORDER = true;
static const bool DBG_SHOW_CURRENT_BITS = true;

static struct TCell *first_memory_cell;
static struct TGlobalCell *front_global_cell;
static struct TLabel *first_label;
static struct TCell *selected_cell;
static struct TCell *prevCell;
static struct TGlobalCell *back_global_cell;
static struct TLabel *curr_label;
static struct TIfElseStatement *current_if_else_statement;
static struct TDebugState debug_state;
static char *source_program_path;
static char current_instruction;
static long long int program_file_cursor_position = 0;
static bool skip_instruction_because_of_if_else_statement;
static bool last_instruction_was_a_function_call = false;
static bool last_instruction_was_a_return = false;
static long long int n_nested_comments;
static bool show_usage = false;
static short int error = OK;
static bool debug = false;

struct TBit {
	BOOL1_T value;
	/* "Null" bit value is determined by the parameter "next". */
	struct TBit *next;
	struct TBit *prev;
};

/* Memory cell. */
struct TCell {
	/* The first bit (front, LSB to MSB) always exists and is ignored; the
	 * actual selected bit is therefore considered "selected_bit->next";
	 * this is confusing but it allows to have just a boolean for the bit
	 * value, decreasing memory usage in case you use a theoretical
	 * processor or a virtual machine where "bool" is actually treated as
	 * one single bit; the alternative would be an integer to account for
	 * the "null" value. Using a "bit field" structure is also an option but
	 * it's slower in computation and you can't fit the null value anyway.
	 */
	struct TBit ignored_starting_bit;

	struct TBit *selected_bit;
	struct TCell *next;
	struct TCell *prev;
};

/* Memory cell of the global queue. */
struct TGlobalCell {
	/* The first bit always exists and is ignored; see TCell. */
	struct TBit ignored_starting_bit;

	struct TGlobalCell *next;
};

struct TLabel {
	long long int file_pos;
	struct TLabel *next;
	struct TLabel *prev;
};

struct TIfElseStatement {
	bool skip_this_block;
	enum condition_types type;
	bool condition_result;
	struct TIfElseStatement *next_nested;
	struct TIfElseStatement *prev_nested;
};

struct TDebugState {
	struct TCell *dbg_current_cell;
	struct TGlobalCell *dbg_curr_global_cell;
	bool instr_has_immediate_effect_in_memory;
};

bool
my_strcpy(char *destination, char *source, size_t max_length)
{
	size_t srclen = strlen(source);
	if (srclen + 1 < max_length) {
		memcpy(destination, source, srclen + 1);
		return 0;
	} else {
		error = ERR_STRING_TOO_LONG;
		return 1;
	}
}

int
process_arguments(int argc, char *argv[])
{
	int c;
	opterr = 0;
	int non_option_argc;

	static struct option long_options[] = {
	    {"debug", no_argument, NULL, 'd'}, {NULL, 0, NULL, 0}};

	while ((c = getopt_long(argc, argv, "d", long_options, NULL)) != -1) {
		switch (c) {
		case 'd': {
			debug = true;
			break;
		}
		case '?': {
			if (isprint(optopt)) {
				fprintf(
				    stderr, "Unknown option `-%c'.\n", optopt);
			} else {
				fprintf(
				    stderr,
				    "Unknown option character `\\x%x'.\n",
				    optopt);
			}
			return 1;
		}
		default: {
			abort();
		}
		}
	}

	non_option_argc = argc - optind;

	if (non_option_argc == 0) {
		show_usage = true;
		return 0;
	} else if (non_option_argc == 1) {
		source_program_path = argv[optind];
		return 0;
	} else {
		fprintf(stderr, "Too many arguments.\n");
		return 1;
	}
}

void
register_all_labels(FILE *source_program)
{
	while (fscanf(source_program, "%1c", &current_instruction) == 1) {
		if (current_instruction == '{') {
			n_nested_comments++;
		} else if (
		    current_instruction == '}' && n_nested_comments > 0) {
			n_nested_comments--;
		}

		if (n_nested_comments == 0) {
			if (current_instruction == ':') {
				/* Register a new label. */
				struct TLabel *new_label =
				    malloc(sizeof(struct TLabel));
				new_label->next = NULL;
				new_label->file_pos =
				    program_file_cursor_position;

				/* Update pointers. */
				if (first_label == NULL) {
					first_label = new_label;
					curr_label = new_label;
					new_label->prev = NULL;
				} else {
					curr_label->next = new_label;
					new_label->prev = curr_label;
					curr_label = new_label;
				}
			}
		}
		program_file_cursor_position++;
	}
}

void
execute_source_program_function(FILE *source_program, long long int from_pos)
{
	struct TCell cells;
	struct TCell *backup_pointer_of_the_selected_cell;
	struct TIfElseStatement *backup_pointer_of_current_if_else_statement;
	long long int backup_source_program_pos;

	first_memory_cell = &cells;
	selected_cell = first_memory_cell;
	selected_cell->ignored_starting_bit.next = NULL;
	selected_cell->ignored_starting_bit.prev = NULL;
	selected_cell->selected_bit = &selected_cell->ignored_starting_bit;
	selected_cell->next = NULL;
	selected_cell->prev = NULL;
	current_if_else_statement = NULL;
	skip_instruction_because_of_if_else_statement = false;

	/* Start from a given position. */
	if (fseek(source_program, from_pos, SEEK_SET) != 0) {
		error = ERR_SEEK_PROGRAM_POSITION;
		process_errors();
		free_local_function_memory();
		clear_if_else_statements();
		return;
	}

	/* Read the instructions one by one. */
	while (fscanf(source_program, "%1c", &current_instruction) == 1) {
		dbg_print_instruction();

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

		process_current_instruction(source_program);

		if (error != OK) {
			process_errors();
			free_local_function_memory();
			clear_if_else_statements();
			return;
		} else if (last_instruction_was_a_function_call) {
			last_instruction_was_a_function_call = false;
			if (first_label == NULL) {
				error = ERR_JUMP_BUT_NO_LABEL;
				process_errors();
				free_local_function_memory();
				clear_if_else_statements();
				return;
			}

			/* Backup. */
			backup_source_program_pos = ftell(source_program);
			backup_pointer_of_the_selected_cell = selected_cell;
			backup_pointer_of_current_if_else_statement =
			    current_if_else_statement;

			/* Call another function. */
			execute_source_program_function(
			    source_program, curr_label->file_pos);

			/* Restore. */
			fseek(
			    source_program, backup_source_program_pos,
			    SEEK_SET);
			first_memory_cell = &cells;
			selected_cell = backup_pointer_of_the_selected_cell;
			current_if_else_statement =
			    backup_pointer_of_current_if_else_statement;

		} else if (last_instruction_was_a_return) {
			last_instruction_was_a_return = false;

			/* Terminate this function. */
			free_local_function_memory();
			clear_if_else_statements();
			return;
		}

		dbg_print_stack_info();
	}
	free_local_function_memory();
	clear_if_else_statements();
}

void
process_current_instruction(FILE *source_program)
{
	/* Check if it's an if-else instruction. */
	switch (current_instruction) {
	case '?': {
		instruction_if_condition_equal_to_1();
		break;
	}
	case '"': {
		instruction_if_condition_equal_to_null();
		break;
	}
	case '!': {
		instruction_else_condition();
		break;
	}
	case ';': {
		instruction_end_of_if_else_statement();
		break;
	}
	}

	skip_instruction_because_of_if_else_statement = true;
	if (current_if_else_statement == NULL) {
		skip_instruction_because_of_if_else_statement = false;
	} else if (
	    current_if_else_statement->skip_this_block == false &&
	    current_if_else_statement->condition_result == true) {
		skip_instruction_because_of_if_else_statement = false;
	}
	if (skip_instruction_because_of_if_else_statement == false) {
		/* Check for other types of instruction. */
		switch (current_instruction) {
		case '>': {
			instruction_go_to_next_cell();
			break;
		}
		case '<': {
			instruction_go_to_previous_cell();
			break;
		}
		case '+': {
			instruction_go_to_next_bit();
			break;
		}
		case '-': {
			instruction_go_to_previous_bit();
			break;
		}
		case '|': {
			instruction_go_to_first_cell();
			break;
		}
		case '=': {
			instruction_go_to_first_bit();
			break;
		}
		case '_': {
			instruction_set_bit_to_zero();
			break;
		}
		case '^': {
			instruction_set_bit_to_one();
			break;
		}
		case '*': {
			instruction_set_bit_to_null();
			break;
		}
		case '%': {
			instruction_set_all_bits_to_null_and_go_to_first_bit();
			break;
		}
		case ']': {
			instruction_print_cell_value_as_ASCII_character();
			break;
		}
		case '[': {
			instruction_get_ASCII_input_and_save_as_cell_value();
			break;
		}
		case '#': {
			instruction_global_queue_enqueue();
			break;
		}
		case '&': {
			instruction_global_queue_dequeue();
			break;
		}
		case '@': {
			instruction_call_function();
			break;
		}
		case '\'': {
			instruction_jump_to_label(source_program);
			break;
		}
		case '/': {
			instruction_select_next_label();
			break;
		}
		case '\\': {
			instruction_select_previous_label();
			break;
		}
		case '$': {
			instruction_select_first_label();
			break;
		}
		case '~': {
			instruction_return();
			break;
		}
		}
	}
}

void
instruction_if_condition_common()
{
	if (current_if_else_statement == NULL) {
		/* Create the first if-else statement. */
		current_if_else_statement =
		    malloc(sizeof(struct TIfElseStatement));
		current_if_else_statement->skip_this_block = false;
		current_if_else_statement->next_nested = NULL;
		current_if_else_statement->prev_nested = NULL;
	} else {
		/* Create a nested if-else statement. */
		struct TIfElseStatement *new_nested_child =
		    malloc(sizeof(struct TIfElseStatement));
		current_if_else_statement->next_nested = new_nested_child;

		/* This new if-else statement can only be executed if the parent
		 * is being executed. */
		if (current_if_else_statement->condition_result == true &&
		    current_if_else_statement->skip_this_block == false) {
			new_nested_child->skip_this_block = false;
		} else
			new_nested_child->skip_this_block = true;

		new_nested_child->next_nested = NULL;
		new_nested_child->prev_nested = current_if_else_statement;

		/* Make the child the current one. */
		current_if_else_statement = new_nested_child;
	}

	current_if_else_statement->type = CONDITION_IF;
}

void
instruction_if_condition_equal_to_1()
{
	instruction_if_condition_common();

	current_if_else_statement->condition_result = false;

	if (selected_cell->selected_bit->next == NULL) {
		current_if_else_statement->condition_result = false;
	} else {
		if (selected_cell->selected_bit->next->value == true) {
			current_if_else_statement->condition_result = true;
		} else {
			current_if_else_statement->condition_result = false;
		}
	}
}

void
instruction_if_condition_equal_to_null()
{
	instruction_if_condition_common();

	current_if_else_statement->condition_result = false;

	if (selected_cell->selected_bit->next == NULL) {
		current_if_else_statement->condition_result = true;
	} else {
		current_if_else_statement->condition_result = false;
	}
}

void
instruction_else_condition()
{
	if (current_if_else_statement == NULL) {
		error = ERR_MISPLACED_ELSE;
		return;
	} else {
		if (current_if_else_statement->type != CONDITION_IF) {
			error = ERR_MISPLACED_ELSE;
		} else {
			current_if_else_statement->type = CONDITION_ELSE;
			current_if_else_statement->condition_result =
			    !current_if_else_statement->condition_result;
		}
	}
}

void
instruction_end_of_if_else_statement()
{
	if (current_if_else_statement == NULL) {
		error = ERR_END_IF;
	} else {
		if (current_if_else_statement->prev_nested == NULL) {
			free(current_if_else_statement);
			current_if_else_statement = NULL;
		} else {
			struct TIfElseStatement *statement_to_delete =
			    current_if_else_statement;
			current_if_else_statement =
			    current_if_else_statement->prev_nested;
			current_if_else_statement->next_nested = NULL;
			free(statement_to_delete);
		}
	}
}

void
instruction_go_to_next_cell()
{
	if (selected_cell->next == NULL) {
		prevCell = selected_cell;
		selected_cell->next = malloc(sizeof(struct TCell));
		selected_cell = selected_cell->next;
		selected_cell->next = NULL;
		selected_cell->prev = prevCell;
		selected_cell->selected_bit =
		    &selected_cell->ignored_starting_bit;
		selected_cell->selected_bit->next = NULL;
		selected_cell->selected_bit->prev = NULL;
	} else {
		selected_cell = selected_cell->next;
	}
}

void
instruction_go_to_previous_cell()
{
	if (selected_cell->prev != NULL) {
		selected_cell = selected_cell->prev;
	}
}

void
instruction_go_to_next_bit()
{
	struct TBit *prev_bit;

	if (selected_cell->selected_bit->next == NULL) {
		prev_bit = selected_cell->selected_bit;
		selected_cell->selected_bit->next = malloc(sizeof(struct TBit));
		selected_cell->selected_bit = selected_cell->selected_bit->next;
		selected_cell->selected_bit->value = false;
		selected_cell->selected_bit->next = NULL;
		selected_cell->selected_bit->prev = prev_bit;
	} else {
		selected_cell->selected_bit = selected_cell->selected_bit->next;
	}
}

void
instruction_go_to_previous_bit()
{
	if (selected_cell->selected_bit->prev != NULL) {
		selected_cell->selected_bit = selected_cell->selected_bit->prev;
	}
}

void
instruction_go_to_first_cell()
{
	selected_cell = first_memory_cell;
}

void
instruction_go_to_first_bit()
{
	selected_cell->selected_bit = &selected_cell->ignored_starting_bit;
}

void
instruction_set_bit_to_zero()
{
	if (selected_cell->selected_bit->next == NULL) {
		struct TBit *new_bit = malloc(sizeof(struct TBit));
		selected_cell->selected_bit->next = new_bit;
		new_bit->value = false;
		new_bit->next = NULL;
		new_bit->prev = selected_cell->selected_bit;
	} else {
		selected_cell->selected_bit->next->value = false;
	}
}

void
instruction_set_bit_to_one()
{
	if (selected_cell->selected_bit->next == NULL) {
		selected_cell->selected_bit->next = malloc(sizeof(struct TBit));
		selected_cell->selected_bit->next->value = true;
		selected_cell->selected_bit->next->next = NULL;
		selected_cell->selected_bit->next->prev =
		    selected_cell->selected_bit;
	} else {
		selected_cell->selected_bit->next->value = true;
	}
}

void
instruction_set_bit_to_null()
{
	selected_cell->selected_bit->next = NULL;
}

void
instruction_set_all_bits_to_null_and_go_to_first_bit()
{
	selected_cell->selected_bit = &selected_cell->ignored_starting_bit;
	selected_cell->selected_bit->next = NULL;
}

void
instruction_print_cell_value_as_ASCII_character()
{
	output(selected_cell);
}

void
instruction_get_ASCII_input_and_save_as_cell_value()
{
	selected_cell->selected_bit = &selected_cell->ignored_starting_bit;
	selected_cell->ignored_starting_bit.next = NULL;
	input(selected_cell);
}

void
instruction_global_queue_enqueue()
{
	if (front_global_cell == NULL) {
		front_global_cell = malloc(sizeof(struct TGlobalCell));
		front_global_cell->ignored_starting_bit.next = NULL;
		front_global_cell->ignored_starting_bit.prev = NULL;
		front_global_cell->next = NULL;
		back_global_cell = front_global_cell;
	} else {
		struct TGlobalCell *new_gl_cell =
		    malloc(sizeof(struct TGlobalCell));
		back_global_cell->next = new_gl_cell;
		back_global_cell = new_gl_cell;
		back_global_cell->ignored_starting_bit.next = NULL;
		back_global_cell->ignored_starting_bit.prev = NULL;
		back_global_cell->next = NULL;
	}

	struct TBit *curr_cell_curr_bit_tmp =
	    &selected_cell->ignored_starting_bit;
	struct TBit *global_curr_bit = &back_global_cell->ignored_starting_bit;
	while (curr_cell_curr_bit_tmp->next != NULL) {
		/* Create a new bit for the global cell. */
		struct TBit *global_new_bit = malloc(sizeof(struct TBit));
		global_curr_bit->next = global_new_bit;
		global_new_bit->value = curr_cell_curr_bit_tmp->next->value;
		global_new_bit->prev = global_curr_bit;

		/* Next */
		curr_cell_curr_bit_tmp = curr_cell_curr_bit_tmp->next;
		global_curr_bit = global_new_bit;
	}
	global_curr_bit->next = NULL;
}

void
instruction_global_queue_dequeue()
{
	if (front_global_cell == NULL) {
		error = ERR_EMPTY_GLOBAL_STACK;
		return;
	}

	struct TBit *curr_cell_starting_bit;
	struct TGlobalCell *global_cell_to_delete;

	free_cell_content(selected_cell);

	/* free_global_cell_content() is slow, just change the TBit pointers. */
	curr_cell_starting_bit = &selected_cell->ignored_starting_bit;
	if (front_global_cell->ignored_starting_bit.next == NULL) {
		/* The cell has null value, */
		curr_cell_starting_bit->next = NULL;
	} else {
		curr_cell_starting_bit->next =
		    front_global_cell->ignored_starting_bit.next;

		/* Give it the new parent. */
		curr_cell_starting_bit->next->prev = curr_cell_starting_bit;
	}
	/* Delete the global cell. */
	global_cell_to_delete = front_global_cell;
	if (front_global_cell->next != NULL) {
		front_global_cell = front_global_cell->next;
	} else {
		front_global_cell = NULL;
	}
	free(global_cell_to_delete);
}

void
instruction_select_next_label()
{
	if (first_label == NULL) {
		error = ERR_LABEL_CURSOR_OUTSIDE_OF_BOUNDS;
		return;
	}
	if (curr_label->next == NULL) {
		error = ERR_LABEL_CURSOR_OUTSIDE_OF_BOUNDS;
		return;
	}
	curr_label = curr_label->next;
}

void
instruction_select_previous_label()
{
	if (first_label == NULL) {
		error = ERR_LABEL_CURSOR_OUTSIDE_OF_BOUNDS;
		return;
	}
	if (curr_label->prev == NULL) {
		error = ERR_LABEL_CURSOR_OUTSIDE_OF_BOUNDS;
		return;
	}
	curr_label = curr_label->prev;
}

void
instruction_select_first_label()
{
	if (first_label == NULL) {
		error = ERR_LABEL_CURSOR_OUTSIDE_OF_BOUNDS;
		return;
	}
	curr_label = first_label;
}

void
instruction_jump_to_label(FILE *source_program)
{
	if (first_label == NULL) {
		error = ERR_JUMP_BUT_NO_LABEL;
		return;
	}
	fseek(source_program, curr_label->file_pos, SEEK_SET);

	clear_if_else_statements();
}

void
instruction_call_function()
{
	last_instruction_was_a_function_call = true;
}

void
instruction_return()
{
	last_instruction_was_a_return = true;
}

void
dbg_print_labels()
{
	if (debug) {
		printf("List of labels:\n");
		if (first_label == NULL) {
			printf("\t(empty)\n");
		} else {
			struct TLabel *curr_label_test = first_label;
			int i = 0;
			while (curr_label_test != NULL) {
				printf(
				    "\tLabel #%d: position: %llu\n", i,
				    curr_label_test->file_pos);
				if (curr_label_test->next == NULL) {
					curr_label_test = NULL;
				} else {
					curr_label_test = curr_label_test->next;
					i++;
				}
			}
		}
		printf("\n");
	}
}

void
dbg_print_instruction()
{
	if (debug == false) {
		return;
	}

	char additional_info[32] = "";
	bool show_symbol = true;
	bool is_comment = false;
	bool is_empty_character = false;

	debug_state.instr_has_immediate_effect_in_memory = true;

	if (n_nested_comments > 0 || current_instruction == '{') {
		my_strcpy(
		    additional_info, "   (comment)", sizeof additional_info);
		is_comment = true;
		debug_state.instr_has_immediate_effect_in_memory = false;
	} else if (skip_instruction_because_of_if_else_statement) {
		my_strcpy(
		    additional_info, "   (skipping execution)",
		    sizeof additional_info);
		debug_state.instr_has_immediate_effect_in_memory = false;
	} else if (current_instruction == '\r') {
		my_strcpy(
		    additional_info, "(carriage return)",
		    sizeof additional_info);
		is_empty_character = true;
		debug_state.instr_has_immediate_effect_in_memory = false;
		show_symbol = false;
	} else if (current_instruction == '\n') {
		my_strcpy(
		    additional_info, "(new line)", sizeof additional_info);
		is_empty_character = true;
		debug_state.instr_has_immediate_effect_in_memory = false;
		show_symbol = false;
	} else if (current_instruction == '\t') {
		my_strcpy(additional_info, "(tab)", sizeof additional_info);
		is_empty_character = true;
		debug_state.instr_has_immediate_effect_in_memory = false;
		show_symbol = false;
	} else if (current_instruction == ' ') {
		my_strcpy(additional_info, "(space)", sizeof additional_info);
		is_empty_character = true;
		debug_state.instr_has_immediate_effect_in_memory = false;
		show_symbol = false;
	} else if (
	    current_instruction == '/' || current_instruction == '\\' ||
	    current_instruction == '$' || current_instruction == ']' ||
	    current_instruction == '?' || current_instruction == '"' ||
	    current_instruction == '!' || current_instruction == ';' ||
	    current_instruction == '\'') {
		/* Leaving out "@" in the condition on purpose: already skipped
		 * by how jumping works; for the same reason, leaving out ":" on
		 * purpose: it will show the new memory. */

		debug_state.instr_has_immediate_effect_in_memory = false;
	}

	if (DBG_SKIP_COMMENTS && is_comment) {
		return;
	} else if (
	    DBG_SKIP_NON_EXECUTED_INSTRUCTIONS &&
	    skip_instruction_because_of_if_else_statement) {
		return;
	} else if (DBG_SKIP_EMPTY_CHARACTERS && is_empty_character == true) {
		return;
	}

	if (show_symbol) {
		printf(
		    "Next instruction: %c%s", current_instruction,
		    additional_info);
	} else {
		printf("Next instruction: %s", additional_info);
	}

	/* Wait for the user to press "Enter". */
	getchar();
}

void
dbg_print_stack_info()
{
	if (debug == false) {
		return;
	}

	if (debug_state.instr_has_immediate_effect_in_memory) {
		printf("\n");
		dbg_print_n_cells(10);

		if (front_global_cell == NULL) {
			printf("(global stack empty)\n");
		} else {
			dbg_print_n_global_cells(10);
		}
	}
}

void
dbg_print_n_cells(int n)
{
	debug_state.dbg_current_cell = first_memory_cell;
	bool go_ahead;
	for (int i = 0; i < n; i++) {
		go_ahead = true;
		if (i > 0) {
			go_ahead = false;
			if (debug_state.dbg_current_cell->next != NULL) {
				debug_state.dbg_current_cell =
				    debug_state.dbg_current_cell->next;
				go_ahead = true;
			}
		}
		if (go_ahead) {
			if (debug_state.dbg_current_cell == selected_cell) {
				printf("> ");
			} else {
				printf("  ");
			}
			printf("Cell #%d: ", i);
			dbg_print_cell_value(debug_state.dbg_current_cell);
			printf("\n");
		}
	}
}

void
dbg_print_n_global_cells(int n)
{
	debug_state.dbg_curr_global_cell = front_global_cell;
	bool go_ahead;
	for (int i = 0; i < n; i++) {
		go_ahead = true;
		if (i > 0) {
			go_ahead = false;
			if (debug_state.dbg_curr_global_cell->next != NULL) {
				debug_state.dbg_curr_global_cell =
				    debug_state.dbg_curr_global_cell->next;
				go_ahead = true;
			}
		}
		if (go_ahead) {
			if (i == 0 &&
			    debug_state.dbg_curr_global_cell->next != NULL) {
				printf("- Global #%d (front): ", i);
			} else if (
			    i > 0 &&
			    debug_state.dbg_curr_global_cell->next == NULL) {
				printf("- Global #%d (back):  ", i);
			} else {
				printf("- Global #%d:         ", i);
			}
			dbg_print_global_cell_value(
			    debug_state.dbg_curr_global_cell);
			printf("\n");
		}
	}
}

void
dbg_print_cell_value_common(
    struct TBit *ignored_starting_bit, struct TBit *selected_bit)
{
	struct TBit *cur = ignored_starting_bit;
	struct TBit *last;
	unsigned int bit_counter = 0;

	char string[255];
	bool too_many_bits = false;

	while (cur->next != NULL) {
		if (DBG_ONLY_PRINT_BITS_IN_READABLE_ORDER == false) {
			if (cur->next->value) {
				if (DBG_SHOW_CURRENT_BITS &&
				    selected_bit == cur) {
					printf("[1]");
				} else {
					printf("1");
				}
			} else {
				if (DBG_SHOW_CURRENT_BITS &&
				    selected_bit == cur) {
					printf("[0]");
				} else {
					printf("0");
				}
			}
		}
		cur = cur->next;
		bit_counter++;
	}
	if (DBG_ONLY_PRINT_BITS_IN_READABLE_ORDER == false) {
		if (DBG_SHOW_CURRENT_BITS && selected_bit == cur) {
			printf("[*]");
		} else {
			printf("*");
		}
	}

	/* Reversed (human readable). */
	unsigned int char_counter = 0;

	last = cur;
	if (selected_bit != NULL) {
		if (selected_bit != last) {
			/* Two brackets. */
			bit_counter += 2;
		}
	}
	if (bit_counter >= sizeof string) {
		too_many_bits = true;
		bit_counter = sizeof string;
	}

	string[bit_counter] = '\0';
	while (cur != ignored_starting_bit) {
		cur = cur->prev;
		if (cur->next->value == true) {
			if (too_many_bits == false && DBG_SHOW_CURRENT_BITS &&
			    selected_bit == cur) {
				string[char_counter] = '[';
				string[++char_counter] = '1';
				string[++char_counter] = ']';
			} else {
				string[char_counter] = '1';
			}
		} else {
			if (too_many_bits == false && DBG_SHOW_CURRENT_BITS &&
			    selected_bit == cur) {
				string[char_counter] = '[';
				string[++char_counter] = '0';
				string[++char_counter] = ']';
			} else {
				string[char_counter] = '0';
			}
		}
		char_counter++;
	}
	if (too_many_bits) {
		/* Print "..." */
		string[sizeof string - 4] = '.';
		string[sizeof string - 3] = '.';
		string[sizeof string - 2] = '.';
		string[sizeof string - 1] = '\0';
	}
	if (DBG_ONLY_PRINT_BITS_IN_READABLE_ORDER == false) {
		if (DBG_SHOW_CURRENT_BITS && selected_bit == last) {
			printf("\t([*]%s)", string);
		} else {
			printf("\t(*%s)", string);
		}
	} else {
		if (DBG_SHOW_CURRENT_BITS && selected_bit == last) {
			printf("[*]%s", string);
		} else {
			printf("*%s", string);
		}
	}
}

void
dbg_print_cell_value(struct TCell *cell)
{
	dbg_print_cell_value_common(
	    &cell->ignored_starting_bit, cell->selected_bit);
}

void
dbg_print_global_cell_value(struct TGlobalCell *gl_cell)
{
	dbg_print_cell_value_common(&gl_cell->ignored_starting_bit, NULL);
}

void
clear_if_else_statements()
{
	struct TIfElseStatement *statement_to_delete;
	while (current_if_else_statement != NULL) {
		statement_to_delete = current_if_else_statement;
		current_if_else_statement =
		    current_if_else_statement->next_nested;
		free(statement_to_delete);
	}
}

void
free_local_function_memory()
{
	struct TCell *next_cell;

	selected_cell = first_memory_cell;
	while (selected_cell->next != NULL) {
		next_cell = selected_cell->next;
		free_cell_content(selected_cell);
		selected_cell = next_cell;
	}
	selected_cell = NULL;
	first_memory_cell = NULL;
}

void
free_global_variables()
{
	struct TGlobalCell *current_g_cell;
	struct TGlobalCell *next_g_cell;
	struct TLabel *next_label;

	/* Free the global queue. */
	if (front_global_cell != NULL) {
		current_g_cell = front_global_cell;
		while (current_g_cell->next != NULL) {
			next_g_cell = current_g_cell->next;
			free_global_cell_content(current_g_cell);
			free(current_g_cell);
			current_g_cell = next_g_cell;
		}
	}

	/* Free registered labels. */
	if (first_label != NULL) {
		curr_label = first_label;
		while (curr_label != NULL) {
			next_label = curr_label->next;
			free(curr_label);
			curr_label = next_label;
		}
		first_label = NULL;
	}
}

void
free_cell_content(struct TCell *cell)
{
	if (cell->ignored_starting_bit.next != NULL) {
		cell->selected_bit = cell->ignored_starting_bit.next;
		while (cell->selected_bit != NULL) {
			struct TBit *bit_to_delete = cell->selected_bit;
			cell->selected_bit = cell->selected_bit->next;
			free(bit_to_delete);
		}
	}
	cell->selected_bit = &cell->ignored_starting_bit;
	cell->selected_bit->next = NULL;
	cell->selected_bit->prev = NULL;
}

void
free_global_cell_content(struct TGlobalCell *g_cell)
{
	struct TBit *tmp_curr_bit;

	if (g_cell->ignored_starting_bit.next != NULL) {
		tmp_curr_bit = g_cell->ignored_starting_bit.next;
		while (tmp_curr_bit != NULL) {
			struct TBit *bit_to_delete = tmp_curr_bit;
			tmp_curr_bit = tmp_curr_bit->next;
			free(bit_to_delete);
		}
	}
}

void
output(struct TCell *cell)
{
	struct TBit *cur = &cell->ignored_starting_bit;
	unsigned int bit_counter = 0;
	char character = 0;
	do {
		if (cur->next == NULL) {
			break;
		} else if (cur->next->value) {
			character += (char)pow(2, bit_counter);
		}
		bit_counter++;
		if (cur->next != NULL) {
			cur = cur->next;
		}
	} while (bit_counter < 127);

	if (debug) {
		printf("OUTPUT: ");
	}

	printf("%c", character);

	if (debug) {
		printf("\n\n");
	}
}

void
input(struct TCell *cell)
{
	char n;
	int remainder;
	struct TBit *prev_bit;

	if (debug) {
		printf("INPUT: ");
	}

	if (scanf("%1c", &n) == 0) {
		error = ERR_USER_INPUT;
		return;
	}

	if (debug) {
		/* Avoid immediately triggering the next print. */
		getchar();

		printf("\n");
	}

	do {
		remainder = n % 2;
		prev_bit = cell->selected_bit;
		struct TBit *newBit = malloc(sizeof(struct TBit));
		cell->selected_bit->next = newBit;
		cell->selected_bit = cell->selected_bit->next;
		cell->selected_bit->value = remainder;
		cell->selected_bit->next = NULL;
		cell->selected_bit->prev = prev_bit;
		n /= 2;
		if (cell->selected_bit->next != NULL) {
			cell->selected_bit = cell->selected_bit->next;
		}
	} while (n != 0);
	cell->selected_bit = &cell->ignored_starting_bit;
}

void
process_errors()
{
	if (error) {
		fprintf(
		    stderr,
		    "\nThe program has been terminated due to an error:\n  ");
		switch (error) {
		case ERR_STRING_TOO_LONG:
			fprintf(stderr, "buffer overflow in some string");
			break;
		case ERR_MISPLACED_ELSE:
			fprintf(stderr, "misplaced else statement");
			break;
		case ERR_END_IF:
			fprintf(
			    stderr,
			    "unexpected end of IF condition or else statement");
			break;
		case ERR_LABEL_CURSOR_OUTSIDE_OF_BOUNDS:
			fprintf(
			    stderr, "label pointer moved outside of bounds");
			break;
		case ERR_JUMP_BUT_NO_LABEL:
			fprintf(
			    stderr, "call or jump to a label, but there is "
				    "no label at all");
			break;
		case ERR_SEEK_PROGRAM_POSITION:
			fprintf(
			    stderr, "can't read from the requested "
				    "position after a jump or "
				    "function call");
			break;
		case ERR_EMPTY_GLOBAL_STACK:
			fprintf(
			    stderr, "tried to pop from the global stack "
				    "but it's empty");
			break;
		case ERR_USER_INPUT:
			fprintf(stderr, "bad input");
			break;
		default:
			fprintf(stderr, "unknown error");
		}
		printf(".\n");

		/* Prevent multiple prints. */
		error = OK;
	}
}

int
main(int argc, char *argv[])
{
	FILE *source_program;

	if (process_arguments(argc, argv) != 0) {
		return 1;
	}

	if (show_usage) {
		printf("Usage: boolx [options] source_file\n");
		printf("\nBoolX official interpreter; v1.0.\n");
		printf("\n  -d                    run the interpreter in "
		       "debug mode\n");
		return 0;
	}

	source_program = fopen(source_program_path, "r");
	if (source_program == NULL) {
		fprintf(stderr, "Can't open the source program file.\n");
		return 1;
	} else {
		front_global_cell = NULL;
		back_global_cell = NULL;

		first_label = NULL;
		curr_label = NULL;

		register_all_labels(source_program);
		curr_label = first_label;

		process_errors();
		if (error != OK) {
			free_global_variables();
			return 1;
		} else {
			dbg_print_labels();

			/* Start the main function of the source program. */
			execute_source_program_function(source_program, 0);

			if (PRINT_NEW_LINE_AFTER_TERMINATION) {
				printf("\n");
			}
		}

		free_global_variables();

		if (error != OK) {
			return 1;
		} else {
			return 0;
		}
	}
	fclose(source_program);
}
