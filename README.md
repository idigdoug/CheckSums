[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Build](https://github.com/idigdoug/CheckSums/actions/workflows/release.yml/badge.svg)](https://github.com/idigdoug/CheckSums/actions/workflows/release.yml)
[![GitHub Release](https://img.shields.io/github/v/release/idigdoug/CheckSums)](https://github.com/idigdoug/CheckSums/releases)

# CheckSums

A fast, lightweight, native Windows command-line tool for computing and validating file checksums.

Works a lot like `md5sum` and `sha256sum` but with extra features including directory recursion,
multiple checksum algorithms, and optional UTF-8 output.

## Table of Contents

- [Why CheckSums?](#why-checksums)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Recursion](#recursion)
- [Base Directory](#base-directory)
- [Output File](#output-file)
- [Checksum Algorithms](#checksum-algorithms)
- [Examples](#examples)
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)

## Why CheckSums?

**CheckSums** is a native Windows binary that:

- **Runs natively on Windows** - no extra runtimes, no WSL, no Cygwin.
- **Supports 12 algorithms** in a single tool, from fast non-cryptographic hashes
  ([Murmur3](https://en.wikipedia.org/wiki/MurmurHash),
  [Adler-32](https://en.wikipedia.org/wiki/Adler-32))
  to cryptographic standards
  ([SHA-256](https://en.wikipedia.org/wiki/SHA-2),
  [SHA-512](https://en.wikipedia.org/wiki/SHA-2)).
- **Recurses into subdirectories** with familiar wildcard patterns.
- **Handles Unicode filenames** correctly with optional UTF-8 BOM output.
- **Is extremely fast** - the default Murmur3x64_128 algorithm is ~11x faster than
  MD5 on typical workloads.

### Sample Output

```
> CheckSums.exe -a crc32 -r pch.*
d61765c7  exe\pch.cpp
e208b879  exe\pch.h
d61765c7  lib\pch.cpp
e208b879  lib\pch.h
d61765c7  test\pch.cpp
1c913623  test\pch.h

> CheckSums.exe -a crc32 -r pch.* -o checksums.crc32

> CheckSums.exe -a crc32 -c checksums.crc32
d61765c7  exe\pch.cpp* OK
e208b879  exe\pch.h* OK
d61765c7  lib\pch.cpp* OK
e208b879  lib\pch.h* OK
d61765c7  test\pch.cpp* OK
1c913623  test\pch.h* OK
```

## Installation

### Requirements

- **Windows 10** or later (x86, x64, ARM64).
- No additional runtime dependencies - the binary is statically linked.

### Download

Download the latest prebuilt binary from the
[Releases](https://github.com/idigdoug/CheckSums/releases) page.

### Build from Source

Requirements: Visual Studio 2022 with the C++ Desktop workload (C++20).

1. Clone the repository: `git clone https://github.com/idigdoug/CheckSums.git`
2. Open `CheckSums.sln` in Visual Studio.
3. Build the solution (Release, x64).

## Quick Start

```
# Compute SHA256 checksums for all files in the current directory:
CheckSums -a SHA256 *

# Record MD5 checksums for all .txt files in the "Files" directory and subdirectories:
CheckSums -a MD5 -r Files\*.txt > ..\checksums.md5

# Validate recorded checksums against the corresponding files:
CheckSums -a MD5 -c ..\checksums.md5
```

## Recursion

When computing checksums, this tool accepts FileSpecs that specify the files to be
processed. The FileSpec is split into a (potentially empty) directory part and a
filename pattern part. The filename pattern may contain wildcards `*` and `?`.

Example: `CheckSums Files\*.txt` will compute checksums for all ".txt" files in the
"Files" directory.

Normally, the filename pattern is evaluated only in the directory specified by the
directory part. However, if the `-r` or `--recurse` option is given, the filename
pattern will be evaluated in every directory at or below the directory specified by
the directory part.

Example: `CheckSums -r Files\*.txt` will compute checksums for all ".txt" files in the
"Files" directory and its subdirectories.

## Base Directory

By default, files to be checksummed are located relative to the current directory.
You can specify a different base directory using the `-d` or `--dir` option. The
output will list file paths relative to the base directory.

Setting the base directory is not the same as specifying the directory in the FileSpec.
Directory components in the FileSpec are included in the output, while the base directory
is not. For example:

```
> CheckSums E:\repos\CheckSums\exe\pch.cpp
b2e755768937966436f669ad8e4411ce  E:\repos\CheckSums\exe\pch.cpp

> CheckSums -d E:\repos\CheckSums exe\pch.cpp
b2e755768937966436f669ad8e4411ce  exe\pch.cpp
```

The base directory is used when computing checksums and when validating checksums.

## Output File

By default, this tool writes results to standard output. However, you can specify an
output file using the `-o` or `--out` option.

This is similar to redirecting output to a file using `>` or `>>`, but may be more
convenient in some cases, especially if file names contain non-ASCII characters.
When using `-o` or `--out`, you can save the results as UTF-8 using the `--utf8bom`
option. You can append to the output file instead of overwriting it using the
`--append` option.

## Checksum Algorithms

This tool supports the following algorithms:

| Algorithm                                                                             | Type                 | Benchmark (relative time, lower = faster) |
| ------------------------------------------------------------------------------------- | -------------------- | ----------------------------------------- |
| [Adler32](https://en.wikipedia.org/wiki/Adler-32)                                     | Non-cryptographic    | 131                                       |
| [Crc32](https://en.wikipedia.org/wiki/Cyclic_redundancy_check)                        | Non-cryptographic    | 779                                       |
| [Fnv1a32](https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function) | Non-cryptographic    | 442                                       |
| [Fnv1a64](https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function) | Non-cryptographic    | 425                                       |
| [MD4](https://en.wikipedia.org/wiki/MD4)                                              | Cryptographic (weak) | 450                                       |
| [MD5](https://en.wikipedia.org/wiki/MD5)                                              | Cryptographic (weak) | 759                                       |
| [Murmur3x64_128](https://en.wikipedia.org/wiki/MurmurHash)                            | Non-cryptographic    | 68 _(default)_                            |
| [SHA1](https://en.wikipedia.org/wiki/SHA-1)                                           | Cryptographic (weak) | 575                                       |
| [SHA256](https://en.wikipedia.org/wiki/SHA-2)                                         | Cryptographic        | 232                                       |
| [SHA384](https://en.wikipedia.org/wiki/SHA-2)                                         | Cryptographic        | 685                                       |
| [SHA512](https://en.wikipedia.org/wiki/SHA-2)                                         | Cryptographic        | 695                                       |
| [Xor64](https://en.wikipedia.org/wiki/XOR_cipher)                                     | Non-cryptographic    | 23                                        |

> **Note:** Benchmark values are relative times measured on a single machine. They are
> useful for comparing algorithms to each other, not as absolute performance numbers.

The default algorithm is **Murmur3x64_128**. This is not a cryptographic hash, but it
is very fast and has good distribution properties. It is suitable for detecting changes
to files.

For security-sensitive applications, use a cryptographic hash algorithm such as SHA256
or SHA512, e.g. `-a sha256`.

> **Note:** The Xor64 algorithm is extremely fast but is very weak. It is not recommended
> for general use but may be useful for benchmarking, i.e. for measuring I/O overhead
> against time spent computing the actual checksum.

## Examples

`CheckSums -a MD5 * > ..\checksums.md5`

This will create a file called `checksums.md5` in the parent directory containing the
MD5 checksums of all files in the current directory. It is essentially equivalent to
running `md5sum * > ..\checksums.md5`.

`CheckSums -a MD5 -c ..\checksums.md5`

This will validate the checksums in `checksums.md5` against the files in the current
directory. It is essentially equivalent to running `md5sum -c ..\checksums.md5`.

`CheckSums -a SHA256 -d c:\MyDir -r * > MyDir.sha256`

This will create a file called `MyDir.sha256` containing the SHA256 checksums of all
files in `c:\MyDir` and all of its subdirectories. The files will be listed as paths
relative to `c:\MyDir`.

`CheckSums -a SHA256 -d c:\MyDir -c MyDir.sha256`

This will validate the checksums in `MyDir.sha256` against the files in `c:\MyDir\...`.

## Usage

```
CheckSums: Compute or check checksums of files.

Computing checksums:

  CheckSums <common and compute options...> <FileSpecs...>

  Writes checksum values and file names to output.

  Each FileSpec is either a file name or a wildcard pattern (e.g. "*.txt").

  If no FileSpecs are given or if FileSpec is "-", reads standard input.

  If the -r or --recurse option is given, FileSpecs will be evaluated
  recursively with behavior like "dir /s /b FileSpec":

  - If FileSpec has no slashes, the base directory (see the -d option) will be
    the search starting point. Otherwise, the base directory will be combined
    with the part of FileSpec before the last slash to form the search
    starting point.
  - The rest of FileSpec (the part after the last slash, or all of FileSpec if
    it has no slashes) will be used as the search pattern. It may contain
    wildcards '*' and '?'.
  - The search pattern will be evaluated in every directory at or below the
    search starting point.

  For example "CheckSums -d C:\Data -r Files\*.txt" will compute checksums for
  all ".txt" files in "C:\Data\Files" and its subdirectories.

Compute options:

  -r, --recurse    Recursively process files in subdirectories.

Checking checksums:

  CheckSums -c      <common and check options...> <ListFiles...>
  CheckSums --check <common and check options...> <ListFiles...>

  If no ListFiles are given or if ListFile is "-", reads standard input.

  Each ListFile should contain lines of the form "<checksum>  <filename>" or
  "<checksum> *<filename>".

Check options:

  --ignore-missing Don't fail or report status for missing files.
  --quiet          Don't write output for successfully-verified files.
  --status         Don't write any output. Status code indicates success.
  --strict         Error for improperly-formatted lines in ListFiles.
  -w, --warn       Warn for improperly-formatted lines in ListFiles.

Common options:

  -a <algorithm>   Use the specified checksum algorithm (e.g. -a sha256).
  -d <directory>   Set the base directory (default is current directory).
  -o, --out <file> Write output to the specified file instead of stdout.
  -b, --binary     Use " *" instead of "  " as the separator between checksum
                   and file name in output.
  -t, --text       Use "  " instead of " *" as the separator between checksum
                   and file name in output (default).
  --append         Append to the -o file instead of overwriting it.
  --utf8bom        Use UTF-8-BOM encoding for the -o file (default is ANSI).
  --unbuffered     Read file data using unbuffered I/O (usually slower).
  --verbose        Verbose logging on stderr.
  --version        Show version information and exit.
  -h, -?, --help   Show this help message and exit.

  The -d directory is used when locating files but is not included in the
  output. For example, "CheckSums -d C:\Data Files\Test.txt" would read from
  "C:\Data\Files\Test.txt" but the output would be "<SUM>  Files\Test.txt".

  The -b and -t options only affect the output format. Files to be checksummed
  are always read as binary (no newline normalization).

Checksum algorithm options, along with a benchmark time (smaller is faster):

    -a Adler32          131
    -a Crc32            779
    -a Fnv1a32          442
    -a Fnv1a64          425
    -a MD4              450
    -a MD5              759
    -a Murmur3x64_128    68 (default)
    -a SHA1             575
    -a SHA256           232
    -a SHA384           695
    -a SHA512           685
    -a Xor64             23
```

## Contributing

Contributions are welcome! Please
[open an issue](https://github.com/idigdoug/CheckSums/issues) to report bugs or
suggest features, or submit a
[pull request](https://github.com/idigdoug/CheckSums/pulls).

## License

Copyright (c) Doug Cook.

Licensed under the [MIT License](LICENSE).
