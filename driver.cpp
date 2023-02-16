#include "./waveform.hpp"
#include <iostream>

int main() {

	wav::Waveform original("sine.wav");
	original.print_header(std::cout);
	// original.print_data(std::cout, demo::formats::dec_format, 32, 0);

	auto clip = original;
	auto gain = original;

	clip.filter(demo::filters::Clip<short>(INT16_MAX / 4));
	clip.save("clip.wav");

	// std::cout << "\n> Contents of 'clip.wav':" << std::endl;
	// clip.print_data(std::cout, demo::formats::dec_format, 32, 0);

	gain.filter(demo::filters::Gain<short>(0.4));
	gain.save("gain.wav");

	// std::cout << "\n>Contents of 'gain.wav':" << std::endl;
	// gain.print_data(std::cout, demo::formats::dec_format, 32, 0);

	// auto max_original = original.maximum_amplitude();
	// auto max_clip = clip.maximum_amplitude();
	// auto max_gain = gain.maximum_amplitude();

	// std::cout << "Maximum sample intensity in 'audio.wav': " << max_original << ", "
	// 		  << (float)max_original / (float)INT16_MAX << std::endl;
	// std::cout << "Maximum sample intensity in 'clip.wav': " << max_clip << ", "
	// 		  << (float)max_clip / (float)INT16_MAX << std::endl;
	// std::cout << "Maximum sample intensity in 'gain.wav': " << max_gain << ", "
	// 		  << (float)max_gain / (float)INT16_MAX << std::endl;

	return EXIT_SUCCESS;
}