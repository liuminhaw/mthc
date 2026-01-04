# mthc

**mthc** is a Markdown-to-HTML converter written in C. 

> Version 1.0.3

---

- [Overview](#overview)
- [Installation](#installation)
    - [Debian based Linux](#debian-based-linux)
    - [RedHat based Linux](#redhat-based-linux)
    - [Arch Linux](#arch-linux)
    - [Building from source](#building-from-source)
        - [Runtime Requirements](#runtime-requirements)
- [Usage](#usage)
- [Supported Syntax](#supported-syntax)
    - [Basic Syntax](#basic-syntax)
    - [Extended Syntax](#extended-syntax)
- [Feedback](#feedback)

---

## Overview
`mthc` currently supports the Markdown *Basic Syntax* as defined in the [Markdown Guide](https://www.markdownguide.org/basic-syntax/), plus one *Extended Syntax* feature: *automatic heading IDs*. Additional extensions are planned for future releases.

It ships with a built-in stylesheet based on the [Catppuccin color palette](https://catppuccin.com/palette/) and supports light/dark mode. Code blocks are highlighted with [highlight.js](https://highlightjs.org/). The CSS/JS required for styling and highlighting are embedded directly in the generated HTML. If you prefer a bare document, you can disable styling to produce plain HTML.

> **Note:** Custom stylesheets are not yet supported. As a workaround, disable the built-in styles and add your own CSS manually.


## Installation
Prebuilt packages are available for Debian/Ubuntu, RHEL/CentOS/Rocky/Alma, and Arch Linux (x86_64/amd64). Download them from the [GitHub Releases page](https://github.com/liuminhaw/mthc/releases).

You can also build from source with `make` (see below).

### Debian based Linux
Download the `.deb` that matches your OS release, then install:

    sudo apt install ./mthc-{version}-{release}.{distro}_{arch}.deb

### RedHat based Linux
Download the `.rpm` that matches your OS release, then install:

    sudo dnf install ./mthc-{version}-{release}.{distro}.{arch}.rpm

### Arch Linux
Download the `.pkg.tar.zst` package, then install:

    sudo pacman -U ./arch-mthc-{version}-{release}-{arch}.pkg.tar.zst

### Building from source
1. Get the source for vX.Y.Z from the [release page](https://github.com/liuminhaw/mthc/releases) or clone the tag:
        git clone --depth 1 --branch vX.Y.Z https://github.com/liuminhaw/mthc.git
1. Install the required build dependencies:
    - `gcc` (C compiler)
    - `libunistring-dev`
    - `libpcre2-dev`
1. Build the executable:
        make

#### Runtime Requirements
Your system must have `libunistring` and `libpcre2-8` installed for `mthc` to run. If install from a package, these dependencies will be handled automatically.

## Usage
Using `mthc` is simple. Just provide a Markdown file and mthc writes HTML to standard output. Use `--output` to write to a file.

    mthc [options] &ltmarkdown_file&gt
     
    Options:
      --help             Show this help message
      --output=FILE      Specify output html file (default: stdout)
      --no-style         Disable CSS styling in the output HTML
      --debug            Enable debug logging
      --test             For testing purposes only
      --version          Show version information

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
