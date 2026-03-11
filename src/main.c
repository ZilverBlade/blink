#ifdef WIN32
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "blink.h"


int main(int argc, char** argv, char** env) {
#ifdef WIN32
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    if (bk_initialize() != BK_SUCCESS) {
        return EXIT_FAILURE;
    }

    /* Set a file input. */
    FILE* input = fopen("D:/Directories/Documents/VisualStudio2019/Projects/blink/examples/test2.bk.yaml", "rb");
    bk_machine machine;
    if (bk_create_interpreter(&machine) != BK_SUCCESS) {
        printf("Failed to create blink interpreter\n");
        return EXIT_FAILURE;
    }

    bk_stream stream = {
        .pFile = input
    };
    bk_unit main;
    if (bk_compile_translation_unit(machine, &stream, "main", &main) != BK_SUCCESS) {
        printf("ERROR: %s\n", bk_interpreter_get_error(machine));
        return EXIT_FAILURE;
    }

    bk_integer len = 0;
    if (bk_emit_translation_unit(main, NULL, &len) != BK_SUCCESS) {
        printf("EMISSION FAILURE\n");
        return EXIT_FAILURE;
    }
    char* outputBuff = calloc(len + 1, 1);
    if (bk_emit_translation_unit(main, outputBuff, &len) != BK_SUCCESS) {
        return EXIT_FAILURE;
    }
    printf("%s\n", outputBuff);

    bk_engine engine;
    if (bk_create_execution_engine(&engine) != BK_SUCCESS) {
        printf("Failed to create blink execution engine\n");
        return EXIT_FAILURE;
    }

    if (bk_execute(engine, &main, 1, 0) != BK_SUCCESS) {
        printf("ERROR: %s\n", bk_engine_get_error(engine));
        return EXIT_FAILURE;
    }

    bk_destroy_execution_engine(engine);
    bk_release_translation_unit(main);
    bk_destroy_interpreter(machine);

    bk_terminate();

    fclose(input);

    return EXIT_SUCCESS;
}