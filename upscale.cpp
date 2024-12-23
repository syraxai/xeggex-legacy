#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include "servers/queue.h"  // Assuming servers and queue are implemented here

using json = nlohmann::json;

// Function to convert a binary image buffer to Base64
std::string bufferToBase64(const std::vector<uint8_t>& buffer) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string base64;
    int i = 0, val = 0, valb = -6;
    for (uint8_t c : buffer) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            base64.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) base64.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (base64.size() % 4) base64.push_back('=');
    return base64;
}

// Function to decode Base64 to a binary buffer
std::vector<uint8_t> base64ToBuffer(const std::string& base64) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::vector<uint8_t> buffer;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[base64_chars[i]] = i;

    int val = 0, valb = -8;
    for (uint8_t c : base64) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            buffer.push_back((val >> valb) & 0xFF);
            valb -= 8;
        }
    }
    return buffer;
}

// Upscale function
std::vector<uint8_t> upscale(
    const std::vector<uint8_t>& imageBuffer,
    int resizeFactor = 2,
    const std::string& upscaler1 = "R-ESRGAN 4x+ Anime6B",
    const std::string& upscaler2 = "None",
    float mixFactor = 0.5
) {
    // Convert the image buffer to Base64
    std::string base64InputImage = "data:image/png;base64," + bufferToBase64(imageBuffer);

    // Create the payload
    json payload = {
        {"resize_mode", 0},
        {"show_extras_results", true},
        {"gfpgan_visibility", 0},
        {"codeformer_visibility", 0},
        {"codeformer_weight", 0},
        {"upscaling_resize", resizeFactor},
        {"upscaling_crop", true},
        {"upscaler_1", upscaler1},
        {"upscaler_2", upscaler2},
        {"extras_upscaler_2_visibility", mixFactor},
        {"upscale_first", false},
        {"image", base64InputImage}
    };

    // Get the first server
    const auto& server = servers[0];

    // Prepare API endpoint and credentials
    std::string apiEndpoint = server.address + "/sdapi/v1/extra-single-image";
    std::string credentials = server.credentials;
    std::string base64Credentials = bufferToBase64(std::vector<uint8_t>(credentials.begin(), credentials.end()));

    // Perform the POST request
    cpr::Response response = cpr::Post(
        cpr::Url{apiEndpoint},
        cpr::Body{payload.dump()},
        cpr::Header{
            {"Content-Type", "application/json"},
            {"Authorization", "Basic " + base64Credentials}
        }
    );

    // Handle the response
    if (response.status_code != 200) {
        throw std::runtime_error("Failed to upscale image. HTTP Status: " + std::to_string(response.status_code));
    }

    json responseData = json::parse(response.text);
    if (!responseData.contains("image")) {
        throw std::runtime_error("Unexpected response format: " + response.text);
    }

    // Convert the Base64 image back to a buffer
    return base64ToBuffer(responseData["image"]);
}

int main() {
    // Example usage
    try {
        // Load an image file into a buffer (replace "input.png" with your image path)
        std::ifstream file("input.png", std::ios::binary);
        std::vector<uint8_t> imageBuffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        // Upscale the image
        std::vector<uint8_t> resultBuffer = upscale(imageBuffer);

        // Save the result to a file
        std::ofstream outFile("output.png", std::ios::binary);
        outFile.write(reinterpret_cast<char*>(resultBuffer.data()), resultBuffer.size());
        std::cout << "Upscaled image saved to output.png" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
