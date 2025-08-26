# mthc
A simple markdown to html converter written in C

## Requirements
`libunistring` and `libpcre2-8` need to be installed on the system to run the program.

## Build
### Install dependencies
Dependencies required for building:
- `libunistring-dev`
- `libpcre2-dev`

### `make`
- Build executable: `make mthc`
- Build with debug information: `make debug`
- Clean build files: `make clean`
- Run tests:
    - Run conversion tests: `make check`
    - Run memory leak tests: `make mem-check`

#### `make` dependencies
Required tools for specific `make` targets:
- `column` (For `make help` formatting)
- `xxd` (For css embedding `make styles`)

## Reference
- [Markdown guide - Basic Syntax](https://www.markdownguide.org/basic-syntax/)
