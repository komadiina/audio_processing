#include "./waveform.hpp"
#include <iostream>

int main(int argC, char** argV)
{
    if (argC != 2) {
        std::cerr << "Bad commandline arguments, exiting..." << std::endl;
        return EXIT_FAILURE;
    }

    // Expected fileformat is: 16-bit mono PCM WAVE
    wav::Waveform audio(argV[1]);

    // Print the header information and the waveform sample values
    audio.print_header();

    // Print 64 samples in hex format (each sample is 2 bytes long)
    // audio.print_data(std::cout, demo::formats::hex_format, 64, 0);

    // Showcase save functionality
    // audio.save("original_" + std::string(argV[1]));

    // Perfom filtering on the provided waveform
    auto modulated = audio;
    modulated.convolute({ 0.1f, 0, -0.2f, 0, 0.3f, 0, -0.1f, 0, 0.01f, 0, -0.1f, 0, 0.1111f });
    modulated.normalize();
    modulated.save("modulated_" + std::string(argV[1]));

    // // Apply a gain filter of 0.5 factor (ratio)
    // modulated = audio;
    // modulated.filter(demo::filters::Gain<short>(0.3));
    // modulated.save("gain_" + std::string(argV[1]));

    // // Clip the modulated waveform to a third of its maximum value
    // modulated = audio;
    // modulated.filter(demo::filters::Clip<short>(INT16_MAX / 3));
    // modulated.save("clip_" + std::string(argV[1]));

    // modulated = audio;
    // modulated.normalize();
    // modulated.save("normalized_" + std::string(argV[1]));

    // // Pulsify all samples above the given threshold of 0.5 absolute amplitude
    // // modulated = audio;
    // modulated.filter(demo::filters::Pulsify<float, short>(0.2f));
    // modulated.save("pulse_" + std::string(argV[1]));

    return EXIT_SUCCESS;
}