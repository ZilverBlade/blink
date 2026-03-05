#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "yaml/yaml.h"

#include "blink.h"


#define PRINT(fmt, ...) printf("%s" fmt "\n", indent, __VA_ARGS__)

int main(int argc, char** argv, char** env) {
    /* Set a file input. */
    FILE* input = fopen("D:/Directories/Documents/VisualStudio2019/Projects/blink/examples/simple.bk.yaml", "rb");

#if 0
    yaml_parser_t parser;
    yaml_event_t event;

    int done = 0;

    /* Create the Parser object. */
    yaml_parser_initialize(&parser);


    yaml_parser_set_input_file(&parser, input);

    char indent[256];
    memset(&indent, 0, 256);
    int indentoff = 0;

    /* Read the event sequence. */
    while (!done) {

        /* Get the next event. */
        if (!yaml_parser_parse(&parser, &event)) {
            printf("Parser error: %d", (int)parser.error);
            return 0;
        }
        memset(indent, ' ', indentoff * 4);
        memset(indent + indentoff * 4, 0, 1);

        switch (event.type) {
        case YAML_ALIAS_EVENT:
            PRINT("alias: %s", event.data.alias.anchor);
            break;
        case YAML_SCALAR_EVENT:
            PRINT("scalar: %s", event.data.scalar.value);
            break;
        case YAML_STREAM_START_EVENT:
            PRINT("stream start");
            ++indentoff;
            break;
        case YAML_STREAM_END_EVENT:
            PRINT("stream end");
            --indentoff;
            break;
        case YAML_DOCUMENT_START_EVENT:
            PRINT("document start");
            ++indentoff;
            break;
        case YAML_DOCUMENT_END_EVENT:
            PRINT("document end");
            --indentoff;
            break;
        case YAML_SEQUENCE_START_EVENT:
            PRINT("sequence start");
            ++indentoff;
            break;
        case YAML_SEQUENCE_END_EVENT:
            PRINT("sequence end");
            --indentoff;
            break;
        case YAML_MAPPING_START_EVENT:
            PRINT("mapping start");
            ++indentoff;
            break;
        case YAML_MAPPING_END_EVENT:
            PRINT("mapping end");
            --indentoff;
            break;
        default:
            PRINT("unknown %d", (int)event.type);
            break;
        }

        /* Are we finished? */
        done = (event.type == YAML_STREAM_END_EVENT);

        /* The application is responsible for destroying the event object. */
        yaml_event_delete(&event);

    }

    /* Destroy the Parser object. */
    yaml_parser_delete(&parser);

#endif

#if 1
    bk_machine machine;
    if (bk_create_interpreter(&machine) != BK_SUCCESS) {
        printf("Failed to create blink interpreter\n");
        return EXIT_FAILURE;
    }
   
    bk_stream stream = {
        .pFile = input
    };
    bk_unit main;
    if (bk_compile_translation_unit(machine, &stream, &main) != BK_SUCCESS) {
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
#endif

    fclose(input);

    return EXIT_SUCCESS;
}