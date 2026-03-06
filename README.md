[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

# CheckSums

Create or validate file checksums on Windows.
Works a lot like `md5sum` and `sha256sum` but with a few extra features:

- Can optionally recurse into subdirectories.
- Can optionally write results to a file instead of stdout.
- Can optionally encode output file as UTF-8 with BOM.
- Supports many different checksum algorithms (see below).

## Recursion

When computing checksums, this tool accepts FileSpecs that specify the files to be
processed. The FileSpec is split into a (potentially empty) directory part and a
filename pattern part. The filename pattern may contain wildcards '*' and '?'.

Example: `CheckSums Files\*.txt` will compute checksums for all ".txt" files the
"Files" directory.

Normally, the filename pattern is evaluated only in the directory specified by the
directory part. However, if the `-r` or `--recurse` option is given, the filename
pattern will be evaluated in every directory at or below the directory specified by
the directory part.

Example: `CheckSums -r Files\*.txt` will compute checksums for all ".txt" files the
"Files" directory and its subdirectories.

## Output file

By default, this tool writes results to standard output. However, you can specify an
output file using the `-o` or `--out` option.

This is similar to redirecting output to a file using `>` or `>>`, but may be more
convenient in some cases, especially if file names contain non-ASCII characters.
When using `-o` or `--out`, you can save the results as UTF-8 using the `--utf8bom`
option. You can append to the output file instead of overwriting it using the
`--append` option.

## Checksum algorithms

This tool supports the following algorithms:

- Adler32
- Crc32
- Fnv1a32
- Fnv1a64
- MD4
- MD5
- Murmur3x64_128
- SHA1
- SHA256
- SHA384
- SHA512
- Xor64

The default algorithm is Murmur3x64_128. This is not a cryptographic checksum algorithm, but it
is very fast and has good distribution properties. It is suitable for detecting changes to files.

For security-sensitive applications, you should select a cryptographic hash algorithm such as SHA256
or SHA512 using the `-a` option.

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

## License

Copyright (c) Doug Cook.

Licensed under the [MIT License](LICENSE).
