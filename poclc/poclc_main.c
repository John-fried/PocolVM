/* poclc_main.c -- PocolC Compiler Main Entry Point */

/* Copyright (C) 2026 Bayu Setiawan and Rasya Andrean
   This file is part of Pocol, the Pocol Virtual Machine.
   SPDX-License-Identifier: MIT
*/

#include "poclc.h"
#include <errno.h>
#include <libgen.h>

/* Print usage information */
static void print_usage(const char *program) {
    printf("PocolC - High-Level Language to PocolVM Compiler\n");
    printf("Usage: %s [options] <input.pc> [-o <output.pob>]\n", program);
    printf("\nOptions:\n");
    printf("  -o <file>     Specify output file\n");
    printf("  -v, --verbose Enable verbose output\n");
    printf("  -h, --help    Show this help message\n");
    printf("\nExample:\n");
    printf("  %s hello.pc -o hello.pob\n", program);
}

int main(int argc, char **argv) {
    const char *input_path = NULL;
    const char *output_path = NULL;
    int verbose = 0;
    
    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_path = argv[++i];
            } else {
                fprintf(stderr, "Error: -o requires an argument\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            input_path = argv[i];
        } else {
            fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    /* Check for input file */
    if (!input_path) {
        fprintf(stderr, "Error: No input file specified\n");
        print_usage(argv[0]);
        return 1;
    }
    
    /* Generate default output path if not specified */
    if (!output_path) {
        /* Replace .pc extension with .pob */
        size_t len = strlen(input_path);
        output_path = malloc(len + 5);
        strcpy((char*)output_path, input_path);
        
        /* Change extension */
        char *dot = strrchr((char*)output_path, '.');
        if (dot && strcmp(dot, ".pc") == 0) {
            strcpy(dot, ".pob");
        } else {
            strcat((char*)output_path, ".pob");
        }
    }
    
    if (verbose) {
        printf("PocolC Compiler\n");
        printf("Input:  %s\n", input_path);
        printf("Output: %s\n", output_path);
        printf("\n");
    }
    
    /* Open input file */
    FILE *input = fopen(input_path, "r");
    if (!input) {
        fprintf(stderr, "Error: Cannot open input file '%s': %s\n", 
                input_path, strerror(errno));
        return 1;
    }
    
    /* Open output file */
    FILE *output = fopen(output_path, "wb");
    if (!output) {
        fprintf(stderr, "Error: Cannot open output file '%s': %s\n", 
                output_path, strerror(errno));
        fclose(input);
        return 1;
    }
    
    /* Initialize compiler context */
    CompilerCtx ctx;
    init_lexer(&ctx, input, input_path);
    ctx.output = output;
    ctx.output_path = output_path;
    
    if (verbose) {
        printf("Parsing...\n");
    }
    
    /* Parse the program */
    AstNode *ast = parse_program(&ctx);
    
    if (ctx.error_count > 0) {
        fprintf(stderr, "Compilation failed with %d error(s)\n", ctx.error_count);
        free_ast(ast);
        fclose(input);
        fclose(output);
        return 1;
    }
    
    if (verbose) {
        printf("Generating bytecode...\n");
    }
    
    /* Generate bytecode */
    generate_code(&ctx, ast);
    
    if (verbose) {
        printf("Compilation successful!\n");
        if (ctx.warning_count > 0) {
            printf("Warnings: %d\n", ctx.warning_count);
        }
    }
    
    /* Cleanup */
    free_ast(ast);
    free_lexer(&ctx);
    fclose(input);
    fclose(output);
    
    /* Free allocated output path if we allocated it */
    if (!argv[2] || strcmp(argv[1], "-o") != 0) {
        free((void*)output_path);
    }
    
    return 0;
}
