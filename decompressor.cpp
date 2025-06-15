#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <zlib.h>
#include <openssl/sha.h>
#include <filesystem>

namespace fs = std::filesystem;

// Computes salted SHA-256 hash of filename for consistent hashed output name
std::string sha256(const std::string& input) {
    const std::string salt = "$packerx_";
    std::string salted = salt + input;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(salted.c_str()), salted.size(), hash);
    std::ostringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return ss.str();
}

// Decompresses file, extracts extension, validates CRC32 checksum
std::vector<unsigned char> decompressFile(const std::string& fullPath, std::string& recoveredExtension, bool& validCRC) {
    std::ifstream file(fullPath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open: " << fullPath << std::endl;
        return {};
    }

    // Read full binary content
    std::vector<unsigned char> compressedData((std::istreambuf_iterator<char>(file)), {});
    file.close();

    if (compressedData.size() < 5) {
        std::cerr << "File too small or corrupted." << std::endl;
        return {};
    }

    // First byte stores extension length
    size_t extLength = compressedData[0];

    // Sanity check on extension and CRC presence
    if (compressedData.size() < 1 + extLength + 4) {
        std::cerr << "Invalid or corrupted metadata." << std::endl;
        return {};
    }

    // Extract original file extension
    recoveredExtension = std::string(compressedData.begin() + 1, compressedData.begin() + 1 + extLength);

    // Extract embedded CRC32 from last 4 bytes
    size_t crcPos = compressedData.size() - 4;
    uLong extractedCRC = 0;
    extractedCRC |= static_cast<uLong>(compressedData[crcPos]);
    extractedCRC |= static_cast<uLong>(compressedData[crcPos + 1]) << 8;
    extractedCRC |= static_cast<uLong>(compressedData[crcPos + 2]) << 16;
    extractedCRC |= static_cast<uLong>(compressedData[crcPos + 3]) << 24;

    // Isolate the compressed payload
    std::vector<unsigned char> actualData(compressedData.begin() + 1 + extLength, compressedData.begin() + crcPos);

    // Prepare buffer for decompressed output
    uLongf destSize = actualData.size() * 4;
    std::vector<unsigned char> uncompressedData(destSize);
    int result = uncompress(uncompressedData.data(), &destSize, actualData.data(), actualData.size());

    // Retry with larger buffer if necessary
    if (result == Z_BUF_ERROR) {
        destSize *= 2;
        uncompressedData.resize(destSize);
        result = uncompress(uncompressedData.data(), &destSize, actualData.data(), actualData.size());
    }

    if (result != Z_OK) {
        std::cerr << "Decompression failed with code: " << result << std::endl;
        return {};
    }

    // Trim buffer to actual size
    uncompressedData.resize(destSize);

    // Compute CRC32 of decompressed data
    uLong actualCRC = crc32(0L, Z_NULL, 0);
    actualCRC = crc32(actualCRC, uncompressedData.data(), uncompressedData.size());
    std::cout<<"Debug --> Actual CRC32: "<<actualCRC<<", Extracted CRC32: "<<extractedCRC<<"\n";
    // Compare computed and embedded CRCs
    validCRC = (actualCRC == extractedCRC);
    return uncompressedData;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: decompress <original_filename>" << std::endl;
        return 1;
    }

    // Extract filename from argument and compute salted SHA-256
    std::string Filename = argv[1];
    std::string ogFilename = fs::path(Filename).stem().string();
    std::string hashedName = sha256(ogFilename);

    // Locate compressed file
    std::string inputFile = "compressed_output/" + hashedName + ".bin";

    std::string recoveredExt;
    bool crcMatch = false;

    // Perform decompression
    std::vector<unsigned char> decompressedData = decompressFile(inputFile, recoveredExt, crcMatch);

    if (decompressedData.empty()) {
        std::cerr << "Decompression failed." << std::endl;
        return 1;
    }

    // Abort if CRC32 check fails
    if (!crcMatch) {
        std::cerr << "CRC32 check FAILED. File may be corrupted." << std::endl;
        return 1;
    }

    // Build output path
    std::string outPath = "decompressed_output/" + ogFilename + "_restored." + recoveredExt;

    // Save restored file
    std::ofstream out(outPath, std::ios::binary);
    out.write(reinterpret_cast<const char*>(decompressedData.data()), decompressedData.size());
    out.close();

    // Final status
    std::cout << "Decompression successful!" << std::endl;
    std::cout << "Restored file: " << outPath << std::endl;
    std::cout << "Extension: ." << recoveredExt << std::endl;
    std::cout << "Size: " << decompressedData.size() << " bytes" << std::endl;

    return 0;
}
