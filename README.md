[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

# CheckSums

Create and validate file/directory checksums. Works a lot like `md5sum` and `sha256sum` but with a few extra features:

- Can optionally recurse into subdirectories.
- Can write directly to a file instead of stdout.
- Can optionally encode output as UTF-8 with BOM.
- Supports many different hashing algorithms (see below).

Supported hashing algorithms:

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

The default hashing algorithm is Murmur3x64_128. This is not a cryptographic hashing algorithm, but it
is very fast and has good distribution properties. It is suitable for detecting changes to files. For
security-sensitive applications, you should use a cryptographic hashing algorithm like SHA256.

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
CheckSums: Compute or check hash values of files.

To compute hashes: CheckSums [-r] <FileSpecs...>

    Writes hash values and file names to output, one per line.
    Each filespec is either a file name or a wildcard pattern (e.g. "*.txt").
    If no FileSpecs are given, hashes stdin.
    FileSpecs may not contain slashes. Use -d to specify a base directory.

    -r             Recursively process files in subdirectories.

To check hashes: CheckSums <options...> -c <ListFiles...>

    Each ListFile should contain lines of the form "<hash> <filename>".
    For each file that doesn't match the expected hash, write the actual hash
    value and file name to output, one per line.

    -s             Don't write results. Status code indicates success/failure.
    -w             Warn about improperly formatted lines in ListFiles.
    -q             Write output only for failures.

Common options:

    -a <algorithm> Use the specified hash algorithm (e.g. -a sha256).
    -d <directory> Set the base directory (default is current directory).
    -o <file>      Write to the specified file instead of stdout.
    -+             Append to the -o file instead of overwriting it.
    -u             UTF-8-BOM encoding for the -o file (default is ANSI).
    -v             Verbose output on stderr.

Hash algorithm options, along with a benchmark time (smaller is faster):

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
