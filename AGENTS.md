# Notes for AI agents working on AutoTrace

These instructions are for coding agents (and humans) changing this repository.
They complement [README.md](README.md) and [TRANSLATING.md](TRANSLATING.md). If
anything here conflicts with a maintainer's explicit instruction, the
maintainer wins.

## Every finding becomes an issue

Any bug you find gets a public GitHub issue, whatever you were doing at the time:
a crash, wrong output, a build or packaging problem, a misleading message. That
applies whether you found it while working on something else, reviewing a pull
request, or triaging, and whether or not you intend to fix it. Security
vulnerabilities are the one exception (see below).

- **No time limit.** You may work on a fix in a private branch first and open
  the issue when you're ready. But the issue must exist **no later than the pull
  request that fixes it**, and the pull request must reference it (`Closes #N`
  or `Refs #N`). A fix with no issue leaves no trace of what was broken.
- **A finding that exists only in a review comment, a commit message or an
  agent's chat transcript is lost.** For example, the library's missing link
  dependencies were reported inside a comment on #124 in 2024, but only got
  their own issue (#324) two years later.
- **Search first**, including closed issues (`gh issue list --state all
  --search ...`). If a closed issue covers it but the bug is back, or was never
  fully fixed, open a new issue that links the old one, rather than silently
  commenting on a closed thread.
- **Write it so that someone else can act on it:** exact reproduction steps
  using files anyone can create (for example, `magick` commands rather than
  private test files), expected versus actual behaviour, the cause with
  `file:line` if known, and whether you confirmed it by running it or only by
  reading the code.
- **Credit earlier reporters and earlier attempts at a fix** (issues, closed
  pull requests).

### Security vulnerabilities

[SECURITY.md](SECURITY.md) is the policy: what counts as a vulnerability, how to
report one, and how it is fixed and disclosed. For agents, it comes down to this:

- **Don't open a public issue or pull request.** Report through private
  vulnerability reporting, and develop the fix in the advisory's private fork. A
  public "fix overflow in X" pull request discloses the bug just as surely as an
  issue does.
- **Don't decide on your own that something is harmless.** Check every finding
  against SECURITY.md's scope before filing it. Memory-safety bugs reachable
  from input data are in scope, including uninitialised memory that can reach
  the output. Wrong output, NULL-pointer crashes and assertion failures are not.
- **When in doubt, report privately.** A private report can be made public in a
  minute, but a public one can't be taken back. Say in the report why you think
  it is or isn't exploitable; the maintainer decides.

## Versioning and releases

### Who does what

- **Only the maintainer tags and publishes releases.** Agents never create or
  push tags, never create GitHub releases, and never upload release assets.
- Agents *prepare* a release: they open the release pull request described
  below, and test the artifacts once the maintainer has tagged.

### The version lives in exactly one place

`AC_INIT([autotrace],[X.Y.Z])` in `configure.ac` is the only source of the
package version. Everything else derives from it: `AUTOTRACE_VERSION` in
`config.h`, `at_version()`, `autotrace -version`, `autotrace.pc`, the
`make dist` tarball name, and the `Project-Id-Version` header of
`po/autotrace.pot`.

- **Never change the version outside the release commit.** Don't bump it "in
  advance" in a feature PR or a separate PR, and don't add `-dev` or `-git`
  suffixes. Between releases, master keeps reporting the last released
  version.
- Never hard-code a version number anywhere else in the sources.
- The Windows installers get their version from the **tag name**, not from
  `configure.ac` (see `.github/workflows/windows.yml`). Builds from
  untagged pushes are named after the date instead, for example
  `autotrace-20261109-win64-setup.exe`.

### Choosing the next version number

- Releases are `0.31.N`. An **API- and ABI-compatible** release bumps `N`, for
  example 0.31.10 to 0.31.11.
- Changing or removing an interface (see below) needs a new minor version and a
  new libtool `CURRENT`. That decision belongs to the maintainer. Propose
  it, never pick it yourself.
- `0.40.0` shows up in old Travis pre-releases (`travis-20200219.*`) and,
  by mistake, inside the `0.31.10` tag's `configure.ac`. It isn't a real
  release line, so don't continue from it.

### What counts as an interface

The public API/ABI of `libautotrace` is exactly:

- the declarations in the installed headers, listed in
  `autotraceinclude_HEADERS` in `Makefile.am` (`types.h`, `autotrace.h`,
  `input.h`, `output.h`, `spline.h`, `exception.h`, `color.h`);
- the size and member layout of every struct, and the values of every enum,
  declared there.

Symbols that `libautotrace.so` happens to export without a declaration in an
installed header are **internal**. Removing them is not an interface change.
One caveat: the `autotrace` program itself may use such a symbol (0.31.10's
binary imports the internal global `logging`). That only breaks a mismatched
old-binary/new-library pair, which distributions don't ship, but mention it in
the release PR.

Some changes are compatible and need only a `REVISION` bump: adding `const` to
a pointer parameter, changing the behaviour of a function behind the same
signature, and reformatting headers.

### Libtool version-info

`LT_CURRENT`, `LT_REVISION` and `LT_AGE` in `configure.ac` feed
`-version-info CURRENT:REVISION:AGE`. The soname is
`libautotrace.so.(CURRENT - AGE)` and is currently `libautotrace.so.3` (3:0:0
as of 0.31.10). Update the triple **once per release, in the release commit**,
following the [libtool rules](https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html):

1. If the library code changed at all since the last release, increment
   `REVISION`.
2. If any interface was added, removed or changed, increment `CURRENT` and
   set `REVISION` to 0.
3. If interfaces were added, increment `AGE`.
4. If interfaces were removed or changed incompatibly, set `AGE` to 0. This
   changes the soname: tell the maintainer before proposing it.

A compatible bug-fix release such as 0.31.11 therefore goes from 3:0:0 to
**3:1:0**. Never touch the triple between releases.

### Checking compatibility before a release

Don't argue compatibility from the header diff, which is mostly formatting
noise. Build the last tag and master side by side in throwaway worktrees, run
`make install` into separate prefixes, and compare them:

1. **Exported symbols:** `nm -D --defined-only libautotrace.so.3`. Any
   removed `at_*` or `autotrace_*` symbol must be checked against the installed
   headers.
2. **Prototypes:** run `gcc -E -P` on `autotrace/autotrace.h` against each
   prefix, extract the `at_*` declarations, normalise the whitespace, and diff
   them.
3. **Layouts:** compile a small program printing `sizeof` and `offsetof` for
   every public struct and member, plus the enum values, once against each
   prefix's headers, and diff the output.
4. **Behaviour:** build a small client against the *old* headers and library
   (`autotrace_init()`, `at_bitmap_read()`, `at_splines_new()`,
   `at_splines_write()`), run it with `LD_LIBRARY_PATH` pointing at the *new*
   library, and require byte-identical output.

If `abidiff` (libabigail) is available, run it too. Put the results in the
release PR description.

### The release pull request

One commit that contains, and only contains:

1. the new version in `AC_INIT` in `configure.ac`;
2. the updated `LT_CURRENT`/`LT_REVISION`/`LT_AGE`;
3. a new `NEWS` entry at the top (user-visible changes and the issues fixed);
4. `po/autotrace.pot` regenerated with `./po/update-pot.sh` (after
   `./autogen.sh && ./configure --without-magick --without-pstoedit`), so that
   its `Project-Id-Version` matches. The `check-pot` CI job only runs when
   sources or the POT change, so it won't catch a stale header for you.
5. the EMF and CGM golden files in `tests/other-output-formats/expected/`,
   regenerated with the new version: from `tests/`, run
   `REGENERATE=1 AUTOTRACE=$PWD/../autotrace sh other-output-formats/run other-output-formats`.
   Both formats embed "AutoTrace version X" in the binary output, which the
   test can't normalise away the way it does for text formats. Check that
   only the version bytes changed (`cmp -l` against the old file).

Before opening it, check locally that `make distcheck` passes and that
`autotrace -version` prints the new version.

### After the maintainer merges and tags

- The tag is the bare version (`0.31.11`, no `v` prefix) and must point at
  the release commit, so that the version inside the tagged sources matches the
  tag. 0.31.10 got this wrong: its tag still said 0.40.0.
- Pushing the tag runs `windows.yml`, which builds the win32 and win64 NSIS
  installers and uploads them to the release itself. The source tarball from
  `make dist` is attached to the release by hand, because GitHub's
  auto-generated archives have no `configure` script.
- Test the uploaded installers on real Windows: install silently with
  `setup.exe /S /D=<dir>`, run `autotrace -version`, then trace a PNG. A
  65 KB `autotrace.exe` means the libtool wrapper script was packaged instead of
  the program (issue #223).
- Close the issues the release fixes, with a comment naming the release.
