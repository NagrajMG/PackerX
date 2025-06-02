# PackerX

A lightweight C++ utility to **compress** and **decompress** files using `zlib`.  
Preserves original file extension as metadata, enabling accurate recovery.

## Directory Structure
```
PackerX/
├── compressor.cpp              # Compression logic
├── decompressor.cpp            # Decompression logic
├── compressed_output/          # Folder for storing compressed files
├── decompressed_output/        # Folder for storing decompressed files
├── README.md                   # Current file
└── input_files/                # Folder for original input files
```
## Build Instructions

Ensure you have a C++17-compatible compiler and `zlib` installed.

## Compile Commands

1. Clone the repository.
2. Compile the program using your C++ compiler, linking against the zlib library.
```bash
g++ -std=c++17 -o compresso compressor.cpp -lz
g++ -std=c++17 -o decompresso decompressor.cpp -lz
```
3. Run the compiled program with the file name you want to compress or decompress using command line.
```bash
./compresso <file_to_compress>
./decompresso <compressed_file>
```
4. Compressed file would be saved in folder `compressed_output`, while decompressed file would be saved in folder `decompresssed_output`.

## Compression Output: Example
```bash
Compression successful!
Output file: compressed_output/example1_compressed.bin
Compressed size: 178 bytes
```

## Decompression Output: Example
```bash
Decompression successful!
Output file: decompressed_output/example1_uncompressed.txt
Original extension: .txt
Decompressed size: 422 bytes
```

## Features

- **Extension-Preserving Compression**  
  Retains original file extension as metadata for accurate decompression.

- **Universal File Support**  
  Works with any file type: `.txt`, `.csv`, `.exe`,`.png`, `.pdf` etc.

- **Robust Error Handling**  
  Gracefully handles:
  - Missing or corrupted files
  - Empty files
  - Common `zlib` errors: `Z_MEM_ERROR`, `Z_BUF_ERROR`, `Z_DATA_ERROR`

- **Self-Descriptive Format**  
  Stores output as:  
  `[1-byte extension length][extension][compressed binary data]`

- **Organized Output**  
  - `compressed_output/` for compressed `.bin` files.
  - `decompressed_output/` for restored files.
