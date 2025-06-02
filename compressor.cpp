#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <zlib.h>
#include <filesystem>
namespace fs = std::filesystem;

std::vector<unsigned char> compressFile(const std::string& filename) {
    // for reading the file as it is, use binary.
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Ran into error: Opening file: " << filename << std::endl;
        return {};
    }
    // Read file contents into a buffer till the end of file
    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});

    // Checking if the file is empty
    if (buffer.empty()) {
        std::cerr << "Oops! Received an empty file." << std::endl;
        return {};
    }
    // Compress the buffer
    uLong sourceSize = static_cast<uLong>(buffer.size());  // zlib demands uLong -- size of original source
    uLong destSize = compressBound(sourceSize);            // worst case size for compressed data
    std::vector<unsigned char> compressed(destSize);       // Memory for compressed result

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
    // Resize the compressed buffer to the actual compressed size
    compressed.resize(destSize);

    return compressed;
}

// Utility to generate output filename without special characters
std::string makeOutputFilename(const std::string& filepath) {
    size_t slash = filepath.find_last_of("/\\");
    std::string filename = (slash == std::string::npos) ? filepath : filepath.substr(slash + 1);

    size_t dot = filename.find_last_of('.');
    std::string base = (dot == std::string::npos) ? filename : filename.substr(0, dot);

    return base + "_compressed.bin";  
}

// Check if the user provided a file path as a command-line argument
int main(int argc, char* argv[]) {
    if (argc < 2) {
        // If not, print usage instructions
        std::cerr << "Usage: " << argv[0] << " <file_path_to_compress>" << std::endl;
        // Exit the program with an error code
        return 1;
    }

    /*
    Arguments from command line
    argv[0] ./file_compression
    argv[1] filename
    */

    std::string filepath = argv[1];

    // Extract file extension (metadata)
    std::string extension;
    size_t dotPos = filepath.find_last_of('.');
    if (dotPos != std::string::npos)
        extension = filepath.substr(dotPos + 1);  // e.g., "txt"
    else
        extension = "bin";  // fallback if no extension

    std::vector<unsigned char> compressedData = compressFile(filepath);
    if (compressedData.empty()) {
        std::cerr << "Failed to compress file." << std::endl;
        return 1;
    }
    // Prepare metadata
    unsigned char extLen = static_cast<unsigned char>(extension.length());
    std::vector<unsigned char> finalOutput;
    finalOutput.push_back(extLen);  // 1-byte length prefix
    finalOutput.insert(finalOutput.end(), extension.begin(), extension.end()); // metadata
    finalOutput.insert(finalOutput.end(), compressedData.begin(), compressedData.end()); // data

    // output filename
    std::string outputFolder = "compressed_output";
    std::string outputFilename = makeOutputFilename(filepath);
    std::string fullOutputPath = outputFolder + "/" + outputFilename;
    std::ofstream outFile(fullOutputPath, std::ios::binary);
    
    if (!outFile) {
        std::cerr << "Could not create output file: " << fullOutputPath << std::endl;
        return 1;
    }

    outFile.write(reinterpret_cast<const char*>(finalOutput.data()), finalOutput.size());
    outFile.close();

    std::cout << "Compression successful!" << std::endl;
    std::cout << "Output file: " << fullOutputPath << std::endl;
    std::cout << "Compressed size: " << finalOutput.size() << " bytes" << std::endl;

    return 0;

}    