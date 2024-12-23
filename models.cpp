#include <iostream>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    try {
        // Perform the GET request
        cpr::Response response = cpr::Get(
            cpr::Url{"http://127.0.0.1:7860/sdapi/v1/sd-models"},
            cpr::Header{{"Content-Type", "application/json"}}
        );

        // Parse the response JSON
        if (response.status_code == 200) {
            json data = json::parse(response.text);

            // Check if the data is an array
            if (data.is_array()) {
                std::cout << "Models retrieved successfully:\n" << data.dump(4) << std::endl;
            } else {
                std::cerr << "Could not get models, /v1/sd-models did not return an array. Response:\n" 
                          << response.text << std::endl;
            }
        } else {
            std::cerr << "Request failed with status code: " << response.status_code 
                      << "\nResponse: " << response.text << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
    }

    return 0;
}
