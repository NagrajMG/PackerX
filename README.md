# PackerX

A lightweight C++ utility to compress and decompress files using `zlib`.
Preserves original file extension in the binary-payload, adds integrity check via `CRC32`, and anonymizes stored/compressed data using salted `SHA-256` hashing.

## Directory Structure
```
PackerX/
├── compressor.cpp              # Compression logic
├── decompressor.cpp            # Decompression + CRC verifier
├── compressed_output/          # Compressed .bin files (hashed names)
├── decompressed_output/        # Recovered files
├── input_files/                # Original raw files
└── README.md                   # This documentation
```
## Build Instructions

Ensure you have a C++17-compatible compiler.
`zlib` and `OpenSSL` installed (libssl-dev on Ubuntu / brew install openssl on macOS)

### Compile Commands

1. Clone the repository.
2. Compile the program using your C++ compiler, linking against the zlib library.
```bash
g++ -std=c++17 -o packerx compressor.cpp -lz -lssl -lcrypto
g++ -std=c++17 -o unpackerx decompressor.cpp -lz -lssl -lcrypto
```
3. Run the compiled program with the file name you want to compress or decompress using command line.
```bash
./packerx input_files/example.txt
./unpackerx input_files/example.txt
```
4. Compressed file would be saved in folder `compressed_output`, while decompressed file would be saved in folder `decompresssed_output`.
### Compression Output: Example
```bash
Compression complete.
Output: compressed_output/f5889fad896cf23c5cc4a9091c9deccd17e39b3ee1fac4cf91b92042328b6cf4.bin
SHA-256 (salted): f5889fad896cf23c5cc4a9091c9deccd17e39b3ee1fac4cf91b92042328b6cf4
Extension embedded: .pdf
CRC32: 0x3E645559
Original size: 4285949 bytes
Compressed size (+meta): 3793879 bytes
```

### Decompression Output: Example
If file is untampered: Result format in terminal.
```bash
Debug --> Actual CRC32: 1046762841, Extracted CRC32: 1046762841
Decompression successful!
Restored file: decompressed_output/Strategy_Insights_References_restored.pdf
Extension: .pdf
Size: 4285949 bytes
```
If file is corrupted/tampered: Result format in terminal.
```bash
CRC32 check FAILED. File may be corrupted.
```

## Features

- **Deterministic File Hashing**  
  Uses salted SHA-256 64 hex character hash of original filename for consistent and reversible file identification.

- **Extension-Preserving Compression**  
  Stores file extension metadata in compressed output, allowing accurate restoration of the original format.

- **CRC32 Checksum Validation**  
  Adds a CRC32 checksum for integrity verification during decompression to detect tampered or corrupted files.

- **Organized Directory Output**  
  Compressed files saved in `compressed_output/`, decompressed files restored to `decompressed_output/`.

- **Robust Compression Logic**  
  Uses `zlib` for efficient compression; handles empty files, memory errors, and corrupted inputs gracefully.

## Future Work

- Create an NPM package for easy integration in web apps.
- Automatically upload compressed files to cloud storage.
- Enable file recovery using a unique hashed ID.
- Support batch compression of multiple files.
- Add password protection for secure file access.
