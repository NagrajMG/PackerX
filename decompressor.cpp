#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <zlib.h>

std::vector<unsigned char> decompressFile(const std::string& filename, std::string& recoveredExtension){
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Ran into error: Opening file: "<< filename << std::endl;
        return {};
    }
    std::vector<unsigned char> compressedData(std::istreambuf_iterator<char>(file), {});
    file.close();
    
    if (compressedData.empty()) {
        std::cerr << "Oops! Received an empty file." << std::endl;
        return {};
    }
    // Extract metadata
    size_t extLength = compressedData[0];
    if (extLength + 1 > compressedData.size()) {
        std::cerr << "Invalid metadata in file." << std::endl;
        return {};
    }

    recoveredExtension = std::string(compressedData.begin() + 1, compressedData.begin() + 1 + extLength);

    // Extract actual compressed data
    std::vector<unsigned char> actualData(compressedData.begin() + 1 + extLength, compressedData.end());

    // Try to guess initial uncompressed size (start with 4× compressed size)
    uLongf destSize = actualData.size() * 4;
    std::vector<unsigned char> uncompressedData(destSize);

    int result = uncompress(uncompressedData.data(), &destSize, actualData.data(), actualData.size());

    // If too small, try again with 8× size
    if (result == Z_BUF_ERROR) {
        destSize = actualData.size() * 8;
        uncompressedData.resize(destSize);
        result = uncompress(uncompressedData.data(), &destSize, actualData.data(), actualData.size());
    }

    if (result != Z_OK) {
        std::cerr << "Decompression failed (zlib error code: " << result << ")" << std::endl;
        return {};
    }

    uncompressedData.resize(destSize);
    return uncompressedData;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        // If not, print usage instructions
        std::cerr << "Usage: " << argv[0] << " <compressed_file_path>" << std::endl;
        // Exit the program with an error code
        return 1;
    }
    std::string filepath = argv[1];
    std::string recoveredExt;
    std::vector<unsigned char> decompressedData = decompressFile(filepath, recoveredExt);
    if (decompressedData.empty()) {
        std::cerr << "Failed to de-compress file." << std::endl;
        return 1;
    }
    // Build output filename
    std::string baseName = filepath.substr(filepath.find_last_of("/\\") + 1);
    size_t pos = baseName.find("_compressed.bin");
    std::string nameBase = (pos != std::string::npos) ? baseName.substr(0, pos) : "restored_file";
    std::string outputFile = "decompressed_output/" + nameBase + "_uncompressed." + recoveredExt;
    std::ofstream out(outputFile, std::ios::binary);
    if (!out) {
        std::cerr << "Could not write to output file: " << outputFile << std::endl;
        return 1;
    }
    out.write(reinterpret_cast<const char*>(decompressedData.data()), decompressedData.size());
    out.close();
    // Terminal output
    std::cout << "Decompression successful!" << std::endl;
    std::cout << "Output file: " << outputFile << std::endl;
    std::cout << "Original extension: ." << recoveredExt << std::endl;
    std::cout << "Decompressed size: " << decompressedData.size() << " bytes" << std::endl;
}