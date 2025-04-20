# Totality Encrypted Archive (TEA) format

## Overview

TEA file is generally composed of a header section and a data section. Data section appears first in the binary file only after first signature section. The next is the header section and the last is the second signature section.

- First signature section: 6 bytes
- Data section: n bytes + 1 byte
- Header section: n bytes + n bytes + n bytes + 46 bytes
- Second signature section: 3 bytes

## First signature section

- 3 bytes indicating the start of the TEA file (by default `TEA` in ASCII)
- 1 byte of padding
- 1 byte indicating the version of the TEA format used in the file
- 1 byte of padding

## Second signature section

- 3 bytes indicating the end of the TEA file (by default `TEA` in ASCII)

## Data section

- n bytes of data, where n is the total size of files in the archive
- 1 byte of padding

## Header section

- n bytes of data header, where n is determined by the size of the data header
- 1 byte of padding
- n bytes of common flags, where n is dependent on the number of files in the archive and the number of flags
- 1 byte of padding
- n bytes of metadata, where n is the total size of metadata in the archive
- 1 byte of padding
- 42 bytes of the whole archive header
- 1 byte of padding

### Data header

Data header is a structure that describes the files in the archive.

- 8 bytes of total size of all file headers
- n bytes of file headers, where n is the total size of all file headers
- n * 8 bytes of position of each file header in the data header, where n is the number of files in the archive

#### File header
File header is a structure that describes the file in the archive.

- 2 bytes of file name length
- n bytes of file name
- 8 bytes of file size
- 8 bytes of the parent position in the archive (`0xFFFFFFFFFFFFFFFF` if the file is in the root directory)
- 8 bytes of the file offset from the start of data section
- 8 bytes of epoch modification time
- 1 byte reserved

### Common flags

Common flags are a set of flags that apply to all files in the archive. The number of common flags is determined by the number of files in the archive and the number of flags.

- n bytes of common flags, where n is determined in the archive header

Bit usage (b1 b2 b3 b4 b5 b6 b7 b8, ...):
- b1: if set, the file is encrypted
- b2: if set, the file is compressed
- b3: indicates the compression algorithm used
- b4: indicates the compression algorithm used
- b5: indicates the compression strength
- b6: indicates the compression strength
- b7: if set, the file is a directory
- b8>=: reserved

### Metadata

Metadata can be anything that is not part of the data section. It is also used for storing DEFLATE metadata for quick continous compression.

### Archive header

- 8 bytes of number of files in the archive
- 8 bytes of position of the data header in the archive
- 2 bytes of number of common flags
- 8 bytes of position of the common flags in the archive
- 2 bytes of size of the metadata
- 8 bytes of position of the metadata in the archive
- 2 bytes of global flags
- 4 bytes reserved

#### Global flags

Bit usage (b1 b2 b3 b4 b5 b6 b7 b8 b9 b10 b11 b12 b13 b14 b15 b16):
- b1: if set, the archive is encrypted
- b2: if set, the archive is compressed
- b3: indicates the compression algorithm used
- b4: indicates the compression algorithm used
- b5: indicates the compression strength
- b6: indicates the compression strength
- b7>=: reserved
