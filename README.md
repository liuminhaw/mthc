# mthc

A Markdown-to-HTML converter written in C

This README focuses on development. For general usage and full documentation, visit the [mthc webpage](https://mthc.lmhaw.dev).

## Development

### Build
Build with `make` is the recommended way to compile `mthc`.

Required build dependencies:
- `libunistring-dev`
- `libpcre2-dev`

#### Dependencies
Required tools for specific `make` targets: 
- `column` (For `make help` formatting)
- `xxd` (For css embedding `make styles`)
- `valgrind` (For memory leak tests `make mem-check`)

#### Common targets
- Build executable: `make mthc` or just `make`
- Clean build files: `make clean`
- Run tests:
  - Run conversion tests: `make check`
  - Run memory leak tests: `make mem-check`
- Generate styles: `make styles`
- For more targets, run `make help`

## Releasing package

### Debian
Update version / release with changelog in `debian/changelog` (can use `dch` command for this)

### RPM
Update version / release with changelog in `rhel/rpm.spec`

### Arch Linux
Update version / release in `PKGBUILD`

