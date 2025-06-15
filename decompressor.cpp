#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <zlib.h>
#include <openssl/sha.h>
namespace fs = std::filesystem;

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

std::vector<unsigned char> decompressFile(const std::string& fullPath, std::string& recoveredExtension) {
    std::ifstream file(fullPath, std::ios::binary);
    if (!file) {
        std::cerr <<"Ran into error: Opening file: " << fullPath << std::endl;
        return {};
    }

    std::vector<unsigned char> compressedData((std::istreambuf_iterator<char>(file)), {});

    if (compressedData.empty()) {
        std::cerr << "Empty file." << std::endl;
        return {};
    }

    size_t extLength = compressedData[0];
    recoveredExtension = std::string(compressedData.begin() + 1, compressedData.begin() + 1 + extLength);
    std::vector<unsigned char> actualData(compressedData.begin() + 1 + extLength, compressedData.end());
    uLongf destSize = actualData.size() * 4;
    std::vector<unsigned char> uncompressedData(destSize);
    int result = uncompress(uncompressedData.data(), &destSize, actualData.data(), actualData.size());
    
    if (result == Z_BUF_ERROR) {
        destSize *= 2;
        uncompressedData.resize(destSize);
        result = uncompress(uncompressedData.data(), &destSize, actualData.data(), actualData.size());
    }

    if (result != Z_OK) {
        std::cerr << "Decompression failed, code: " << result << std::endl;
        return {};
    }

    uncompressedData.resize(destSize);
    return uncompressedData;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: decompress <original_filename>" << std::endl;
        return 1;
    }

    std::string Filename = argv[1];
    std::string ogFilename = fs::path(Filename).stem().string(); 
    std::string hashedName = sha256(ogFilename); // hash of original filename
    std::string inputFile = "compressed_output/" + hashedName + ".bin";

    std::string recoveredExt;
    std::vector<unsigned char> decompressedData = decompressFile(inputFile, recoveredExt);
    if (decompressedData.empty()) {
        std::cerr << "Decompression failed." << std::endl;
        return 1;
    }

    std::string outPath = "decompressed_output/" + ogFilename + "_restored." + recoveredExt;
    std::ofstream out(outPath, std::ios::binary);
    out.write(reinterpret_cast<const char*>(decompressedData.data()), decompressedData.size());
    out.close();

    // Terminal output
    std::cout << "Decompression successful!" << std::endl;
    std::cout << "Original extension: ." << recoveredExt << std::endl;
    std::cout << "Decompressed size: " << decompressedData.size() << " bytes" << std::endl;
    return 0;
}
