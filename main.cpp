#include "lodepng.h"

#include <iostream>
#include <bitset>

std::vector<unsigned char> hide_message_in_png(std::vector<unsigned char> &png_pixels, std::string &message)
{
    std::vector<unsigned char> modified_pixels = png_pixels;

    std::vector<std::bitset<8>> binary_data;
    for (char c : message) {
        std::bitset<8> binary_char(c);
        binary_data.push_back(binary_char);
    }

    size_t data_size = binary_data.size() * 8;  // Size of binary data in bits
    if (data_size > modified_pixels.size()) {
        std::cout << "Not enough bits to hide text in."
            << " You need: " << data_size << " bits."
            << " You have: " << modified_pixels.size() << " bits." << std::endl;
        return modified_pixels;  // Return the original pixels if there is not enough capacity
    }

    size_t binary_data_i = 0;
    size_t binary_data_j = 0;
    for (size_t i = 0; i < modified_pixels.size(); ++i) {
        unsigned char& color_channel = modified_pixels[i];

        bool bit_to_hide = binary_data[binary_data_i][binary_data_j++];
        if (binary_data_j == 8) {
            binary_data_i++;
            binary_data_j = 0;
        }

        color_channel = (color_channel & 0xFE) | bit_to_hide;

        if (binary_data_i >= binary_data.size()) break;
    }

    return modified_pixels;
}

std::string retrieve_message_in_png(const std::vector<unsigned char>& png_pixels)
{
    std::vector<std::bitset<8>> hidden_data;

    std::bitset<8> current_byte;
    size_t binary_data_j = 0;

    for (size_t i = 0; i < png_pixels.size(); ++i) {
        unsigned char color_channel = png_pixels[i];

        // Extract the LSB from the color channel
        bool hidden_bit = color_channel & 0x01;

        // Add the extracted bit to the current byte
        current_byte[binary_data_j++] = hidden_bit;

        if (binary_data_j == 8) {
            // Byte construction complete, push it to the hidden_data vector
            hidden_data.push_back(current_byte);

            // Reset for the next byte
            current_byte.reset();
            binary_data_j = 0;
        }
    }

    // Convert the binary data to characters
    std::string extracted_message;
    for (const std::bitset<8>&binary_char : hidden_data) {
        char character = static_cast<char>(binary_char.to_ulong());
        extracted_message += character;
    }

    return extracted_message;
}

int main(int argc, char* argv[])
{
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <image_file.png> <-e/-d> <message>" << std::endl;
        return 0;
    }

    std::string file_name = argv[1];
    std::string mode = argv[2];
    std::string message;
    if (argc == 4) message = argv[3];

    std::vector<unsigned char> png_file;
    std::vector<unsigned char> png_pixels;
    unsigned width, height, error;
    lodepng::State state;

    state.decoder.color_convert = 0;
    state.decoder.remember_unknown_chunks = 1;

    // Load the PNG image
    if (lodepng::load_file(png_file, file_name) != 0) {
        std::cout << "Failed to load image file: " << file_name << std::endl;
        return 0;
    }

    // Decode the PNG image
    if (lodepng::decode(png_pixels, width, height, state, png_file) != 0) {
        std::cout << "Failed to decode image file: " << file_name << std::endl;
        return 0;
    }

    std::cout << "Width: " << width
        << ", Height: " << height
        << ", Pixels size: " << png_pixels.size() / 4 << std::endl;

    png_file.clear();

    if (mode == "-e") {
        if (argc < 4) {
            std::cout << "Usage: " << argv[0] << " <image_file.png> <-e/-d> <message>" << std::endl;
            return 0;
        }

        if (message.empty()) {
            std::cout << "The message is empty." << std::endl;
            return 0;
        }

        png_pixels = hide_message_in_png(png_pixels, message);

        state.encoder.text_compression = 1;

        error = lodepng::encode(png_file, png_pixels, width, height, state);
        if (error) {
            std::cout << "encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
            return 0;
        }

        lodepng::save_file(png_file, "encoded_" + file_name);
        std::cout << "Encoded image saved in " << "encoded_" + file_name << std::endl;
    } else if (mode == "-d") {
        std::string extracted_message = retrieve_message_in_png(png_pixels);
        std::cout << "Extracted Message: " << extracted_message << std::endl;
    } else {
        std::cout << "Invalid mode. Please use -e to hide a message or -d to retrieve a message." << std::endl;
        return 0;
    }

    return 0;
}