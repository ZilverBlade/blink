#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "yaml/yaml.h"


#define PRINT(fmt, ...) printf("%s" fmt "\n", indent, __VA_ARGS__)

int main(int argc, char** argv, char** env) {
    yaml_parser_t parser;
    yaml_event_t event;

    int done = 0;

    /* Create the Parser object. */
    yaml_parser_initialize(&parser);

    /* Set a file input. */
    FILE* input = fopen("D:/Directories/Documents/VisualStudio2019/Projects/blink/examples/hworld.bk.yaml", "rb");

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

    return EXIT_SUCCESS;
}