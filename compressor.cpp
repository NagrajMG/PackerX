#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <zlib.h>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
namespace fs = std::filesystem;

// Generate salted SHA-256 hash for deterministic hashed filename
std::string sha256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return ss.str();
}

// Compress a file and return the compressed byte vector
std::vector<unsigned char> compressFile(const std::string& filename) {

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Ran into error: Opening file: " << filename << std::endl;
        return {};
    }

    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});

    // Checking if the file is empty
    if (buffer.empty()) {
        std::cerr << "Oops! Received an empty file." << std::endl;
        return {};
    }

    uLong sourceSize = static_cast<uLong>(buffer.size());
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

    compressed.resize(destSize);

    return compressed;
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file_to_compress>" << std::endl;
        return 1;
    }
    std::string filepath = argv[1];
    std::string filenameOnly = fs::path(filepath).stem().string(); 

    // Generate SHA-256 hash from salted filename
    std::string salted = "$packerx_" + filenameOnly;
    std::string hashedName = sha256(salted);

    // Extract extension
    std::string extension = fs::path(filepath).extension().string();

    if (!extension.empty() && extension[0] == '.')
        extension = extension.substr(1);
    if (extension.empty())
        extension = "bin";

    std::vector<unsigned char> compressedData = compressFile(filepath);

    if (compressedData.empty()) return 1;

    //Prepare output: [extLen][ext][compressedData]
    unsigned char extLen = static_cast<unsigned char>(extension.length());
    std::vector<unsigned char> finalOutput;
    finalOutput.push_back(extLen);
    finalOutput.insert(finalOutput.end(), extension.begin(), extension.end());
    finalOutput.insert(finalOutput.end(), compressedData.begin(), compressedData.end());

    //Write to compressed_output/<hash>.bin
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
    std::cout << "Original size: " << fs::file_size(filepath) << " bytes\n";
    std::cout << "Compressed size: " << finalOutput.size() << " bytes\n";

    return 0;
}
