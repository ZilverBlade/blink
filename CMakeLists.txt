cmake_minimum_required(VERSION 3.28)

add_subdirectory(extern)

add_executable(blink 
   src/main.c
   src/blink.c
   src/yaml/api.c
   src/yaml/dumper.c
   src/yaml/emitter.c
   src/yaml/loader.c
   src/yaml/parser.c
   src/yaml/reader.c
   src/yaml/scanner.c
   src/yaml/writer.c
)

