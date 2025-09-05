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

## Packaging

### Debian
Debian `changelog` file common workflows (with `dch`)

- Install `dch`
    ```
    sudo aptt install devscripts
    ```
- Start a new release entry from your working tree:
    ```
    dch -v 1.0.1-1 "Fix install path; add manpage."
    ```
- Bump just the Debian revision (packaging tweak only):
    ```
    # turns 1.0.0-1 into 1.0.0-2
    dch -i "Adjust Build-Depends; fix hardening flags."
    ```
- Mark ready for release (sets date and finalizes):
    ```
    dch -r
    ```


## Reference

- [Markdown guide - Basic Syntax](https://www.markdownguide.org/basic-syntax/)
