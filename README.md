# AutoTrace

[![REUSE status](https://api.reuse.software/badge/github.com/autotrace/autotrace)](https://api.reuse.software/info/github.com/autotrace/autotrace)
[![Coverity passed](https://scan.coverity.com/projects/18456/badge.svg)](https://scan.coverity.com/projects/autotrace-autotrace)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/6255/badge)](https://www.bestpractices.dev/projects/6255)
[![Build status on Linux](https://github.com/autotrace/autotrace/actions/workflows/linux_test.yml/badge.svg)](https://github.com/autotrace/autotrace/actions?query=workflows%3Alinux_test)
[![Build status on macOS](https://github.com/autotrace/autotrace/actions/workflows/macosx_test.yml/badge.svg)](https://github.com/autotrace/autotrace/actions?query=workflows%3Amacosx_test)
[![Build status on FreeBSD](https://github.com/autotrace/autotrace/actions/workflows/freebsd_test.yml/badge.svg)](https://github.com/autotrace/autotrace/actions?query=workflows%3Afreebsd_test)
[![Build status on Windows](https://github.com/autotrace/autotrace/actions/workflows/windows_test.yml/badge.svg)](https://github.com/autotrace/autotrace/actions?query=workflows%3Awindows_test)

AutoTrace is a program for converting bitmap images to vector graphics.

## Features

- **Outline and centerline tracing** - trace the edges or centerlines of shapes
- **Color reduction** - reduce colors for cleaner output
- **Despeckling** - remove noise from images
- **Multiple formats** - supports numerous input formats (PNG, BMP, PNM, TGA, etc.) and output formats (SVG, EPS, PDF, DXF, etc.)

## Installation

### Package Managers

**macOS:**
```bash
brew install autotrace
# or
port install autotrace
```

**Linux:**
```bash
# Fedora/RHEL
sudo dnf install autotrace

# Arch Linux
sudo pacman -S autotrace

# Debian/Ubuntu - build from source (see below)
```

**FreeBSD:**
```bash
pkg install autotrace
```

### Windows

Download the installer from the [Releases page](https://github.com/autotrace/autotrace/releases).

**Available installers:**
- `autotrace-VERSION-win32-setup.exe` (32-bit)
- `autotrace-VERSION-win64-setup.exe` (64-bit)

### Building from Source

See [INSTALL](INSTALL) for detailed instructions.

**Requirements:**
- C compiler (GCC, Clang, MSVC)
- libpng (optional, for PNG support)
- ImageMagick or GraphicsMagick (optional, for additional formats)
- GLib 2.x

**Quick start:**
```bash
# System-wide installation (requires root)
./autogen.sh
./configure
make
sudo make install

# User-local installation (no root needed)
./autogen.sh
./configure --prefix=$HOME/.local
make
make install
```

## Usage

### Command Line

Convert a bitmap to SVG:
```bash
autotrace input.png -output-file output.svg
```

Trace centerlines:
```bash
autotrace input.png -centerline -output-file output.svg
```

Reduce colors before tracing:
```bash
autotrace input.png -color-count 4 -output-file output.svg
```

For complete options, see `autotrace --help` or `man autotrace`.

### Library

AutoTrace can be used as a library (`libautotrace`). Here's a minimal example:

```c
#include <autotrace/autotrace.h>

int main()
{
  at_fitting_opts_type * opts = at_fitting_opts_new();
  at_input_read_func rfunc = at_input_get_handler("image.png");
  at_bitmap_type * bitmap = at_bitmap_read(rfunc, "image.png", NULL, NULL, NULL);
  at_splines_type * splines = at_splines_new(bitmap, opts, NULL, NULL);
  at_output_write_func wfunc = at_output_get_handler_by_suffix("eps");

  at_splines_write(wfunc, stdout, "", NULL, splines, NULL, NULL);
  at_splines_free(splines);
  at_bitmap_free(bitmap);
  at_fitting_opts_free(opts);

  return 0;
}
```

Compile with:
```bash
gcc sample.c $(pkg-config --cflags --libs autotrace)
```

See `autotrace.h` for full API documentation.

## Supported Formats

**Input formats:** PNG, BMP, TGA, PNM (PBM/PGM/PPM), GF
- With ImageMagick/GraphicsMagick: JPEG, TIFF, GIF, and many more

**Output formats:** SVG, EPS, PDF, AI, DXF, CGM, EMF, MIF, ER, Fig, and many others

List all formats: `autotrace --list-input-formats` or `autotrace --list-output-formats`

## Translation

AutoTrace uses [GNU gettext](https://www.gnu.org/software/gettext/) for internationalization. Help translate AutoTrace into your language!

[![Translation status](https://hosted.weblate.org/widget/autotrace/multi-auto.svg)](https://hosted.weblate.org/engage/autotrace/)

**For translators:** Visit our [Weblate project](https://hosted.weblate.org/projects/autotrace/) to start translating. See [TRANSLATING.md](TRANSLATING.md) for detailed guidelines.

**For developers:** When adding translatable strings, wrap them with `_()` and run `./po/update-pot.sh` to update the translation template. See [TRANSLATING.md](TRANSLATING.md) for details.

## Contributing

Contributions welcome! See our [issue tracker](https://github.com/autotrace/autotrace/issues) and [TODO](TODO) file.

Areas where help is needed:
- Bug fixes and improvements
- Documentation
- Additional format support
- Plugin development for image editors
- Your ideas and suggestions!

## License

- **Program:** GPL v2 or later
- **Library (`libautotrace`):** LGPL v2.1 or later
- Input/output modules can be used under either license

## History

AutoTrace was originally developed by Martin Weber. Some code was derived from the `limn` program in GNU fontutils, though most has been completely rewritten.

## Links

- **Homepage:** https://github.com/autotrace/autotrace
- **Issues:** https://github.com/autotrace/autotrace/issues
- **GUI Frontend (inactive):** https://github.com/autotrace/frontline

## Author

Martin Weber (martweb@gmx.net)
