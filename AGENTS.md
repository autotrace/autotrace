# Notes for AI agents working on AutoTrace

These instructions are for coding agents (and humans) changing this repository.
They complement [README.md](README.md) and [TRANSLATING.md](TRANSLATING.md). If
anything here conflicts with a maintainer's explicit instruction, the
maintainer wins.

## Skills

Longer procedures are kept as skills in [`.agents/skills/`](.agents/skills/),
one directory per skill, each with a `SKILL.md` in the
[Agent Skills](https://agentskills.io/) format. Codex, GitHub Copilot and
other agents that support the format find them there. Claude Code finds them
through the symlink `.claude/skills`. An agent without skill support can read
the files as plain checklists.

- [code-quality](.agents/skills/code-quality/SKILL.md): checks before pushing
  a change to the C sources (see
  [below](#before-pushing-a-change-to-the-sources)).
- [choosing-options](.agents/skills/choosing-options/SKILL.md): picking
  `autotrace` options for a particular image, by scoring each candidate
  with `tests/corpus/corpus.py score`.

Put a new skill in `.agents/skills/` and add it to this list.

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

## Before pushing a change to the sources

Go through [.agents/skills/code-quality/SKILL.md](.agents/skills/code-quality/SKILL.md)
(Claude Code loads it as the `code-quality` skill; other agents read it as a
checklist). It is written for any C library. Here it means:

- **Two review bots** comment on pull requests:
  - CodeQL (`github-advanced-security[bot]`; the "CodeQL" and "Analyze (cpp)"
    checks);
  - Codex (`chatgpt-codex-connector[bot]`).

  Read both on the final head. Codex's findings here have been changes in
  what library clients observe: a client's own log handler losing messages
  (#371), a cancel taking effect later (#373), 2-bpp BMP samples changing
  meaning (#327).
- **The library's clients** include Inkscape and the GIMP plugins, so check
  the callbacks of `at_splines_new_full()` (`msg_func`, `notify_progress`,
  `test_cancel`), its documented NULL on cancel (#372) and the `autotrace`
  GLib log domain with a client program, not only with `autotrace`.
- **Formatting:** the "format" job runs `clang-format-20 --dry-run --Werror
  src/*.[ch]`.
- **valgrind:** run it through `./libtool --mode=execute valgrind ./autotrace`.
  `.libs/autotrace` alone may load an installed `libautotrace`.
- **Large test inputs** can be made with ImageMagick, e.g. `magick -size
  3000x3000 plasma:fractal -seed 1 -blur 0x8 -colors 12 -type truecolor
  big.png`.
- **A change to traced output:** run the trace-quality corpus against the
  base branch (`tests/corpus/corpus.py run OLD NEW DIR`, see
  [tests/corpus/README.md](tests/corpus/README.md)) and put its table in
  the pull request. The golden-file tests only say that output changed,
  not whether it got better.

## Who merges a pull request

The maintainer merges pull requests. The one exception is a pull request
that can't change what is built or installed: an agent may merge it itself,
with a rebase merge like every other pull request. Before merging, all of
these must hold on its final head:
- all checks pass;
- neither CodeQL nor Codex has an unresolved finding;
- every file it touches is in the list below.

The files an agent may merge:

1. **Documentation:** any `*.md` file, `doc/`, `TODO`, and the text of the
   man page `autotrace.1`.
2. **Comments in `src/`,** including the doc comments in the installed
   headers, as long as the code itself is unchanged and no
   `SPDX-License-Identifier` or copyright line changes. Check two things:
   - Every changed file has the same code. `gcc -fpreprocessed -dD -E -P`
     removes the comments without expanding macros, and its output for the
     old and new versions must be the same apart from blank lines
     (`diff -B`). Comparing object files doesn't work for this: a comment
     that adds a line shifts `__LINE__` in every assertion below it.
   - No added line contains a backslash or `??`: `git diff OLD NEW -- src |
     grep '^+' | grep -v '^+++' | grep -F -e '\' -e '??'` must print
     nothing. `-fpreprocessed` doesn't join lines, but the compiler does: a
     `//` comment ending in `\` swallows the next line of code. With
     `-std=c17`, which `configure.ac` sets, so does one ending in the
     trigraph `??/`.
3. **Test and tooling code that is never installed:**
   - `tests/corpus/` (its scripts and README, and images that need no
     new `REUSE.toml` entry; a third-party image does, which makes it a
     licensing change);
   - new `tests/github-*` and `tests/other-*` directories that add
     coverage without regenerating any existing golden file.

   `tests/runtests.sh` runs every such directory, and `EXTRA_DIST` already
   ships all of `tests/`, so they need no `Makefile.am` change. A test that
   needs a compiled helper in `tests/helpers/` does need one, so it isn't
   covered.
4. **`.gitignore`.**

Everything else is for the maintainer to merge, even if it is one line:
- any change to code in `src/`, including a message string;
- `configure.ac` and the `Makefile.am` files;
- `.github/` (the workflows, including the spell checker's settings, and
  Dependabot), because CI permissions and secrets are security settings;
- `distribute/` (the Windows installers);
- `po/` (the POT file and the translations);
- licensing: `REUSE.toml`, `LICENSES/`, and any `SPDX-License-Identifier`
  or copyright line, wherever it is;
- regenerated golden files;
- release pull requests and security fixes.

A pull request that mixes the two kinds is the maintainer's as well.

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

- **The next release is 0.40.0.** master now takes interface changes,
  starting with the option for #348. 0.40.0 changes the soname to
  `libautotrace.so.4` (see [Libtool version-info](#libtool-version-info)).
  After it, an **API- and ABI-compatible** release bumps the last number,
  for example 0.40.0 to 0.40.1.
- The 0.31 line ended with 0.31.13 (`libautotrace.so.3`). It only gets
  another release for a serious (high or critical) vulnerability; see
  [Security fixes for 0.31](#security-fixes-for-031).
- Each further interface change (see below) needs a new minor version and a
  new libtool `CURRENT`. That decision belongs to the maintainer. Propose
  it, never pick it yourself.
- `0.40.0` also shows up in old Travis pre-releases (`travis-20200219.*`)
  and, by mistake, inside the `0.31.10` tag's `configure.ac`. Those builds
  are older than 0.31.x and have nothing to do with the real 0.40.0. There
  is no `0.40.0` tag yet, so the name is free.

### Security fixes for 0.31

There is no `0.31.x` branch until one is needed. If a vulnerability of high
or critical severity also affects 0.31.13 (see
[SECURITY.md](SECURITY.md#supported-versions)):

1. The maintainer creates the `0.31.x` branch from the `0.31.13` tag.
2. The fix lands on master as usual, through the advisory's private fork
   (see [SECURITY.md](SECURITY.md)).
3. It is cherry-picked onto `0.31.x` with `git cherry-pick -x`, adapted if
   it doesn't apply. Nothing else goes on that branch: no features, and no
   bug fixes that aren't security fixes.
4. The release (0.31.14 and so on) follows the usual release pull request
   against `0.31.x`, with only `REVISION` bumped, so it stays
   `libautotrace.so.3`.

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
`libautotrace.so.(CURRENT - AGE)`. It is `libautotrace.so.3` from 0.31.10
(3:0:0) to 0.31.13 (3:3:0). 0.40.0 changes interfaces incompatibly, so it
will be 4:0:0, `libautotrace.so.4`. Update the triple **once per release,
in the release commit**, following the [libtool rules](https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html):

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
  installers and uploads them to the release itself. Nothing else is
  attached: the source release is GitHub's automatic archive of the tag,
  which is what distributions package (they run `autoreconf` anyway).
  `make distcheck` is a local check before the release pull request, not a
  release artifact.
- Never delete old releases or pre-releases, or their assets. That includes
  the `travis-20200219.*` builds labelled 0.40.0, which are older than
  0.31.x: something may still download them by URL.
- Test the uploaded installers on real Windows: install silently with
  `setup.exe /S /D=<dir>`, run `autotrace -version`, then trace a PNG. A
  65 KB `autotrace.exe` means the libtool wrapper script was packaged instead of
  the program (issue #223).
- Close the issues the release fixes, with a comment naming the release.
