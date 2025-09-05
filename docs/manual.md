# mthc

**mthc** is a Markdown-to-HTML converter written in C. 

> Version 1.0.0

---

- [Requirements](#requirements)
- [Installation](#installation)
    - [Dependencies](#dependencies)
    - [Build](#build)
- [Usage](#usage)
- [Supported Syntax](#supported-syntax)
    - [Basic Syntax](#basic-syntax)
    - [Extended Syntax](#extended-syntax)
- [Feedback](#feedback)

---

`mthc` currently supports the Markdown *Basic Syntax* as defined in the [Markdown Guide](https://www.markdownguide.org/basic-syntax/), plus one *Extended Syntax* feature: *automatic heading IDs*. Additional extensions are planned for future releases.

It ships with a built-in stylesheet based on the [Catppuccin color palette](https://catppuccin.com/palette/) and supports light/dark mode. Code blocks are highlighted with [highlight.js](https://highlightjs.org/). The CSS/JS required for styling and highlighting are embedded directly in the generated HTML. If you prefer a bare document, you can disable styling to produce plain HTML.

> **Note:** Custom stylesheets are not yet supported. As a workaround, disable the built-in styles and add your own CSS manually.

## Requirements
To run `mthc`, your system must have `libunistring` and `libpcre2-8` installed.

## Installation
Building with `make` is the recommended way to install `mthc`.

Download the source from the **v1.0.0** release or clone the tag directly:

    git clone --depth 1 --branch v1.0.0 https://github.com/liuminhaw/mthc.git

### Dependencies
To build with `make`, you’ll need:
- `gcc` (C compiler)
- `libunistring-dev`
- `libpcre2-dev`

### Build
Build with a single `make` command:

    make

## Usage
Using `mthc` is simple. Just provide a Markdown file and mthc writes HTML to standard output. Use `--output` to write to a file.

    mthc [options] &ltmarkdown_file&gt
     
    Options:
      --help             Show this help message and exit
      --output=FILE      Specify output html file (default: stdout)
      --no-style         Disable CSS styling in the output HTML
      --debug            Enable debug logging
      --test             For testing purposes only

The `--debug` flag writes verbose diagnostic output to standard error and is primarily intended for development.

## Supported Syntax
Currently supported markdown syntax includes:

### Basic Syntax
- Headings (levels 1-6)
- Bold and Italic text
- Blockquotes
- Lists (ordered and unordered)
- Code blocks and inline code
- Links and images
- Horizontal rules
- Raw HTML blocks
    > When using raw HTML in Markdown, the HTML block must be followed by a blank line—even after the closing tag. The parser treats everything from the opening tag up to the next blank line as part of the HTML block.

See the Basic Syntax reference [here](https://www.markdownguide.org/basic-syntax/).

### Extended Syntax
- Heading ID
    > `mthc` automatically generates `id` attributes for headings based on their text. Custom IDs are not supported yet.

## Feedback 
If you have questions about using mthc, feel free to email <liuminhaw@gmail.com>.
To report bugs or request features, please open an issue in the [GitHub repository](https://github.com/liuminhaw/mthc). Pull requests are welcome.

