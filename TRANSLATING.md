# Translating AutoTrace

Thank you for your interest in translating AutoTrace! This guide covers both translators and developers.

## For Translators

### Quick Start

1. Visit our [Weblate project](https://hosted.weblate.org/projects/autotrace/)
2. Sign up for a free Weblate account (or use GitHub/Google login)
3. Select your language or request a new one
4. Start translating!

## Translation Guidelines

### General Principles

- **Be consistent**: Use the same terminology throughout
- **Be concise**: Keep translations similar in length to the English original
- **Context matters**: Some words have different meanings in different contexts
- **Test your work**: If possible, test translations in the actual application

### Special Considerations for AutoTrace

AutoTrace is a command-line tool for converting bitmap images to vector graphics. When translating:

- **Technical terms**: Some terms like "bitmap", "vector", "spline" are often left in English or transliterated
- **Command-line options**: Do not translate the actual option names (like `--output-format`), only their descriptions
- **File formats**: Format names (SVG, PNG, EPS, etc.) should remain unchanged
- **Error messages**: Should be clear and helpful to users trying to solve problems

### Translation Comments

Some strings have comments marked with `TRANSLATORS:` that provide context. These appear in Weblate and help you understand how the string is used.

Example:
```
TRANSLATORS: This appears when the user provides an invalid file path
"Not a valid input file name %s"
```

## String Format Specifiers

AutoTrace uses C-style format specifiers in strings. These must be preserved in your translation:

| Specifier | Meaning | Example |
|-----------|---------|---------|
| `%s` | String | `"File: %s"` → `"ファイル: %s"` |
| `%d` | Integer | `"Count: %d"` → `"数: %d"` |
| `%f` | Float | `"Size: %.2f"` → `"サイズ: %.2f"` |
| `\n` | Newline | Must be preserved |

**Important**: The order and type of format specifiers must match the original!

✅ Correct: `"Processing %d of %d files"` → `"%d/%d 個のファイルを処理中"`  
❌ Wrong: `"Processing %d of %d files"` → `"%s 個のファイルを処理中"` (wrong type)  
❌ Wrong: `"Processing %d of %d files"` → `"ファイルを処理中"` (missing specifiers)  

## Plural Forms

Some languages have different plural rules than English. Weblate handles this automatically.

For example:
- English: 1 file / 2 files (2 forms)
- Japanese: 1 ファイル / 2 ファイル (1 form)
- Polish: 1 plik / 2 pliki / 5 plików (3 forms)

When translating plural forms:
1. Weblate will show you the correct number of plural forms for your language
2. Translate each form appropriately
3. Use the format specifiers correctly in each form

## Fuzzy Translations

A translation marked "fuzzy" means:
- It needs review (possibly outdated)
- It won't be used until the fuzzy flag is removed
- Review and either confirm (remove fuzzy) or update the translation

In Weblate, fuzzy strings are marked with a yellow icon. You can mark/unmark strings as needing review.

## Testing Your Translations

If you want to test your translations locally:

### Prerequisites
```bash
# Install AutoTrace build dependencies
sudo apt-get install build-essential autoconf automake libtool \
                     libglib2.0-dev libpng-dev intltool gettext
```

### Build and Test
```bash
# Clone the repository
git clone https://github.com/autotrace/autotrace.git
cd autotrace

# Build
./autogen.sh
./configure
make

# Test with your language
export LANG=your_language_CODE.UTF-8  # e.g., ja_JP.UTF-8, de_DE.UTF-8
./autotrace --help
./autotrace --version
```

## For Developers

### Adding Translatable Strings

When adding user-facing strings to the code, wrap them with gettext macros:

```c
// Simple string
printf(_("Error: file not found\n"));

// String with format specifiers
printf(_("Processing %d images\n"), count);

// String with context (when same English text needs different translations)
printf(g_dpgettext2(NULL, "verb", "Open"));
printf(g_dpgettext2(NULL, "adjective", "Open"));
```

### Updating the Translation Template

After adding or modifying translatable strings:

```bash
cd po
./update-pot.sh
git add autotrace.pot
git commit -m "Update translation template"
```

The `update-pot.sh` script:
- Extracts all translatable strings from the source code
- Generates/updates the `autotrace.pot` template file
- Adds license information for Weblate compliance

### CI Validation

Our CI automatically checks that the POT file is up-to-date on every pull request. If the check fails, simply run:

```bash
cd po
./update-pot.sh
git add autotrace.pot
git commit --amend --no-edit
git push --force-with-lease
```

### Translation Workflow

1. **Developer** adds translatable strings using `_()` macro
2. **Developer** runs `./update-pot.sh` to update the template
3. **CI** verifies the POT file is current
4. **Weblate** automatically syncs and updates all language files
5. **Translators** translate new strings on Weblate
6. **Weblate** commits translations back to the repository

### Adding Files to Translation

If you create new source files with translatable strings, add them to `po/POTFILES.in`:

```bash
echo "src/new-file.c" >> po/POTFILES.in
```

Then run `./update-pot.sh` to regenerate the template.

## Getting Help

- **Questions about translation**: Ask in Weblate comments on specific strings
- **Technical questions**: Open an issue on [GitHub](https://github.com/autotrace/autotrace/issues)
- **General discussion**: Use GitHub Discussions

## Translation Statistics

You can view translation progress for all languages at:
https://hosted.weblate.org/projects/autotrace/

## Credits

All translators are automatically credited in the project. Your contributions will be visible in:
- Weblate project page
- Git commit history
- Project's contributor list

Thank you for helping make AutoTrace accessible to users worldwide! 🌍
