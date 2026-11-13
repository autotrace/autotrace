# Security policy

AutoTrace reads untrusted image files with parsers written in C. It is used both
as a command-line program and as a library, by editor plugins and by services
that vectorise uploaded images. Security reports are taken seriously.

## Reporting a vulnerability

**Please don't report a vulnerability in a public issue, discussion or pull
request.**

Report it privately through GitHub's
[private vulnerability reporting](https://github.com/autotrace/autotrace/security/advisories/new).
A report becomes a private security advisory.

Please include:

- the AutoTrace version or commit, and how it was built (which input readers:
  native, ImageMagick or GraphicsMagick; with or without pstoedit), plus the
  platform;
- the affected component, for example the BMP reader or the SVG writer;
- a reproducer: the input file, or a script that creates it, and the exact command
  line or library calls;
- the impact you observed: a sanitizer report (ASan/UBSan), valgrind output or a
  backtrace.

## How a report is handled

- **Acknowledgement** within 7 days.
- **The fix** is developed in the advisory's temporary private fork and merged
  when its release is cut. A public pull request would disclose the flaw before
  users can upgrade.
- **Embargo:** the fix, the release and the advisory become public together.
  Reporters are asked to keep the flaw private until then, for at most 90 days
  from the report.
- **Target:** a fix within 30 days for high or critical severity, and with the
  next release otherwise. A fix for a released version gets a release of its
  own promptly; it doesn't wait for unrelated work.
- **The advisory** names the release that ships the fix as the patched version.
- **A CVE** is requested through GitHub for a flaw that affected a release.
- **The NEWS entry** for the release names the advisory.
- **Credit** goes to the reporter, unless they prefer to stay anonymous.

## Supported versions

Fixes go into the latest release. There are no backport branches.

## Scope

In scope: anything reachable from input data or from file names, in the
`autotrace` program or in `libautotrace`:

- memory-safety bugs: out-of-bounds reads or writes, use-after-free, double
  free, an integer overflow leading to one of those, and uninitialised memory
  whose contents can reach the output;
- reading, creating or overwriting files other than the ones the user named;
- code execution through input content, file names or options.

Not vulnerabilities (please report these as ordinary
[issues](https://github.com/autotrace/autotrace/issues)): wrong output, crashes on
a NULL pointer, assertion failures, and slow processing or resource use that
needs no memory corruption. Also out of scope: problems that need an attacker who
can already run commands as the user, and bugs in third-party libraries
(ImageMagick, GraphicsMagick, libpng, pstoedit). Report those upstream. Do tell
us if AutoTrace uses one of them in an unsafe way.

If you're unsure whether something is a vulnerability, report it privately. It
can always be made public afterwards.
