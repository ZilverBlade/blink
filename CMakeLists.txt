cmake_minimum_required(VERSION 3.28)

add_subdirectory(extern)

add_library(blink SHARED
   src/blink.c
   src/internal/strhashmap.c
   src/internal/yaml/api.c
   src/internal/yaml/dumper.c
   src/internal/yaml/emitter.c
   src/internal/yaml/loader.c
   src/internal/yaml/parser.c
   src/internal/yaml/reader.c
   src/internal/yaml/scanner.c
   src/internal/yaml/writer.c
)

add_library(bkstd SHARED
   src/std/bkstdlib.c
)

add_executable(blinkcli
   src/main.c
)

target_link_libraries(blinkcli blink)

