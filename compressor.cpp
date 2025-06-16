#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <zlib.h>                // For compress() and crc32()
#include <filesystem>           // For handling file paths and extensions
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>        // For SHA-256

namespace fs = std::filesystem;

// Computes salted SHA-256 hash of filename for consistent hashed output name
std::string sha256(const std::string& input) {
    const std::string salt = "$packerx_";
    std::string salted = salt + input;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    // the SHA256 function from OpenSSL expects const unsigned char* as input
    SHA256(reinterpret_cast<const unsigned char*>(salted.c_str()), salted.size(), hash);
    std::ostringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return ss.str();
}

// Compress the file content and calculate CRC32 of the original content
std::vector<unsigned char> compressFile(const std::string& filename, uLong& crcOut) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Ran into error: Opening file: " << filename << std::endl;
        return {};
    }

    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});
    file.close();

    if (buffer.empty()) {
        std::cerr << "Oops! Received an empty file." << std::endl;
        return {};
    }

    // Compute CRC32 checksum of original file content
    crcOut = crc32(0L, Z_NULL, 0);
    crcOut = crc32(crcOut, buffer.data(), buffer.size());

    // Compress using zlib
    uLong sourceSize = buffer.size();
    uLong destSize = compressBound(sourceSize);
    std::vector<unsigned char> compressed(destSize);

    int result = compress(compressed.data(), &destSize, buffer.data(), sourceSize);

    if (result != Z_OK) 
    {
    std::cerr << "Compression failed with zlib error code: " << result << std::endl;
    switch (result) 
    {
        case Z_MEM_ERROR:
            std::cerr << "Reason: Not enough memory." << std::endl;
            break;
        case Z_BUF_ERROR:
            std::cerr << "Reason: Output buffer too small." << std::endl;
            break;
        case Z_DATA_ERROR:
            std::cerr << "Reason: Input data corrupted or invalid." << std::endl;
            break;
        default:
            std::cerr << "Reason: Unknown zlib error." << std::endl;
    }
    return {};
    }

    compressed.resize(destSize); // Trim buffer to actual compressed size
    return compressed;

}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file_to_compress>" << std::endl;
        return 1;
    }

    std::string filepath = argv[1];
    std::string filenameOnly = fs::path(filepath).stem().string(); 

    // Hash the filename for anonymized output
    std::string hashedName = sha256(filenameOnly);

    // Extract extension (remove the leading dot)
    std::string extension = fs::path(filepath).extension().string();
    if (!extension.empty() && extension[0] == '.') extension = extension.substr(1);
    if (extension.empty()) extension = "bin"; // Fallback extension

    // Compress and get CRC32
    uLong crc = 0;
    std::vector<unsigned char> compressedData = compressFile(filepath, crc);
    if (compressedData.empty()) return 1;

    // Compose final binary: [1-byte extLen][extension][compressedData][4-byte CRC32]
    unsigned char extLen = static_cast<unsigned char>(extension.length());
    std::vector<unsigned char> finalOutput;
    finalOutput.push_back(extLen);
    finalOutput.insert(finalOutput.end(), extension.begin(), extension.end());
    finalOutput.insert(finalOutput.end(), compressedData.begin(), compressedData.end());

    // Append CRC32 checksum 
    finalOutput.push_back((crc >> 0) & 0xFF);
    finalOutput.push_back((crc >> 8) & 0xFF);
    finalOutput.push_back((crc >> 16) & 0xFF);
    finalOutput.push_back((crc >> 24) & 0xFF);

    // Output file path: compressed_output/<hashed>.bin
    std::string outputFolder = "compressed_output";
    std::string fullOutputPath = outputFolder + "/" + hashedName + ".bin";

    std::ofstream out(fullOutputPath, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to create output file: " << fullOutputPath << std::endl;
        return 1;
    }

    out.write(reinterpret_cast<const char*>(finalOutput.data()), finalOutput.size());
    out.close();

    std::cout << "Compression complete.\n";
    std::cout << "Output: " << fullOutputPath << "\n";
    std::cout << "SHA-256 (salted): " << hashedName << "\n";
    std::cout << "Extension embedded: ." << extension << "\n";
    std::cout << "CRC32: 0x" << std::hex << std::uppercase << crc << std::dec << "\n";
    std::cout << "Original size: " << fs::file_size(filepath) << " bytes\n";
    std::cout << "Compressed size (+meta): " << finalOutput.size() << " bytes\n";

    return 0;
}
