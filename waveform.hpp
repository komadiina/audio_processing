#ifndef _WAV_HEADER_H
#define _WAV_HEADER_H

#include <array>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <math.h>
#include <memory.h>
#include <vector>

/* DEMO SHOWCASE */
namespace demo {
	namespace formats {
		// stream formatting is sticky/persistent
		auto hex_format = [](std::ostream& os) -> std::ostream& { return os << std::hex; };
		auto oct_format = [](std::ostream& os) -> std::ostream& { return os << std::oct; };
		auto dec_format = [](std::ostream& os) -> std::ostream& { return os << std::dec; };
	} // namespace formats
	namespace filters {
		// Note: samples are presented as an alternating signal, meaning -32768 is the lower maximum amplitude
		// and 32767 is the upper maximum amplitude

		// === Define your own basic filters using the following template === //
		// Template assumes that the sample type is the same as effect value type (i.e. short, short)
		// If you are operating on things such as amplitudes, then you NEED to specify arguments as, e.g
		// <float, short>
		template <typename EffectValType, typename SampleType = EffectValType> class MyFilter final {
			private:
				EffectValType criteria;

			public:
				// Initialize the criteria for the filter to be applied (not necessary)
				MyFilter(EffectValType value) : criteria(value) {}

				// Perform filtering on the given sample when called as a functor
				SampleType operator()(SampleType& sample) {}
		};

		template <typename EffectValType, typename SampleType = EffectValType> class Clip final {
			private:
				EffectValType threshold;

			public:
				Clip(EffectValType value) : threshold(value) {}

				SampleType operator()(SampleType sample) {
					int sgn = (sample < 0 ? -1 : 1);

					return std::abs(sample) > threshold ? sgn * threshold : sample;
				}
		};

		template <typename EffectValType, typename SampleType = EffectValType> class Gain final {
			private:
				float factor;

			public:
				Gain(float value) : factor(value) {}

				SampleType operator()(SampleType sample) {
					EffectValType attenuated;

					// Take care of overflow!
					if (std::abs(sample) >= std::numeric_limits<SampleType>::max() - 1 ||
						std::numeric_limits<SampleType>::max() / factor < std::abs(sample))
						return sample;

					return (SampleType)(sample * factor);
				}
		};

		template <typename EffectValType = float, typename SampleType = EffectValType> class Pulsify final {
			private:
				// Absolute amplitudal threshold : [0, 1]
				EffectValType threshold;

			public:
				Pulsify(EffectValType value) : threshold(std::abs(value)) {}
				SampleType operator()(SampleType& sample) {
					// abs(sample_intensity) / maximum_intensity = abs_av
					// sample_intensity / maximum_intensity = amplitude
					float amplitude = (float)sample / (float)std::numeric_limits<SampleType>::max();

					if (std::abs(amplitude) < threshold)
						return sample;

					return (sample < 0 ? std::numeric_limits<SampleType>::min()
									   : std::numeric_limits<SampleType>::max());
				}
		};

		template <typename EffectValType = float, typename SampleType = EffectValType> class Normalize final {
			private:
				float factor;

			public:
				Normalize(float value) : factor(value) {}
				SampleType operator()(SampleType sample) { return (SampleType)(sample * factor); }
		};
	} // namespace filters
} // namespace demo

// x64-windows, in bytes:
//      sizeof(char) == 1
//      sizeof(short) == 2
//      sizeof(int) == 4

/*
Some notes:
	8-bit samples are unsigned bytes (0, 255)
	16-bit samples are signed, 2s-complement integers (-32768, 32767)
	A Wave datastream can contain more than two subchunks (in a non-restricting order), each containing their
own, unique identifiers and sizes

	* We are assuming that the WAVE file is 16-bit per-channel (i.e. 2 bytes, so we can use int16_t, a.k.a.
'short')
	* If the WAVE file is encoded in 24-bit (signed), we must use 3-byte-minimum storage for up to 2^24
different values
	* Maximum dynamic range of a 16-bit PCM encoded WAVE is 96dB (144dB for 24-bit)
*/

namespace wav {
	const char FAILURE = 1;
	const char SUCCESS = 0;
	// clang-format off
    
	typedef struct {
			/* --- RIFF chunk descriptor --- */
			std::array<char, 4> chunk_id;           // := "RIFF"
			int chunk_size;                         // 4 + 8 + SubChunk1Size + 8 + SubChunk2Size
			std::array<char, 4> format;             // "WAVE" for .wav

			/* ------- FMT sub-chunk ------- */
			std::array<char, 4> subchunk1_id;       // := "fmt"
			int subchunk1_size;                     // 16 for PCM
			short audio_format;                     // 1: PCM (uncompressed), != 1: compressed
			short num_channels;                     // 1: mono, 2: stereo ...
			int sample_rate;                        // 8000, 44100, 48000 ...
			int byte_rate;                          // SampleRate * NumChannels * BitsPerSample / 8
			short block_align;                      // NumChannels * BitsPerSample / 8
			short bits_per_sample;                  // 8: 8bpc, 16: 16bpc

			/* ------ DATA sub-chunk ------ */
			std::array<char, 4> subchunk2_id;       // := "data"
			int subchunk2_size;                     // NumSamples * NumChannels * BitsPerSample / 8 == NumberOfDataBytes
	} WAVHeader;

	// clang-format on

	class Waveform {
		private:
			WAVHeader _header;
			std::vector<short> _data;

		public:
			Waveform(std::ifstream& file) {
				init_header(file);
				init_data(file);
			}

			Waveform(const std::string& filename) {
				std::ifstream file(filename, std::ios::binary);

				if (file.is_open() == false)
					throw std::runtime_error("Specified file could not be opened.");

				init_header(file);
				init_data(file);

				file.close();
			}

			Waveform& operator=(const Waveform& rhs) {
				_header = rhs._header;
				_data = rhs._data;

				return *this;
			}

			auto& data() { return _data; }
			auto data() const { return _data; }

			int num_samples() { return _header.subchunk2_size / _header.bits_per_sample; }
			int num_samples() const { return _header.subchunk2_size / _header.bits_per_sample; }

			template <typename Functor> Waveform& filter(Functor action) {
				for (short& sample : _data)
					sample = action(sample);

				return *this;
			}

			int save(const std::string& destination = "out.wav") {
				std::ofstream file(destination, std::ios::binary);

				if (file.is_open() == false) {
					std::cerr << "Error: Could not open " << destination << "." << std::endl;
					return FAILURE;
				}

				if (write_header(file)) {
					std::cerr << "Error: Could not save header data to " << destination << "." << std::endl;
					file.close();

					return FAILURE;
				}

				if (write_data(file)) {
					std::cerr << "Error: Could not save information data to " << destination << "."
							  << std::endl;
					file.close();

					return FAILURE;
				}

				// Caller-cleanup
				file.close();
				std::cout << "Sucessfully saved to " << destination << "." << std::endl;

				return SUCCESS;
			}

			int load(const std::string& source) {
				std::ifstream file(source, std::ios::binary);
				if (file.is_open() == false) {
					throw std::runtime_error("Specified file could not be opened.");
					return FAILURE;
				}
				// Reinitialize the header
				init_header(file);

				// Clear and reinitialize the data container
				_data.clear();
				init_data(file);

				// Callee-cleanup
				file.close();

				return SUCCESS;
			}

			void print_header(std::ostream& os = std::cout) {
				auto to_string = [](const std::array<char, 4>& arr) {
					std::string ret = "";
					for (const auto& elem : arr)
						ret += elem;

					return std::string(ret);
				};

				auto chunk_id = to_string(_header.chunk_id);
				auto format = to_string(_header.format);
				auto subchunk1_id = to_string(_header.subchunk1_id);
				auto subchunk2_id = to_string(_header.subchunk2_id);

				os << std::setw(24) << "- RIFF chunk descriptor -" << std::endl;
				os << std::setw(18) << "chunk_id: " << chunk_id << std::endl;
				os << std::setw(18) << "chunk_size: " << _header.chunk_size << std::endl;
				os << std::setw(18) << "format: " << format << std::endl;

				os << std::setw(24) << "\n- FMT sub-chunk - " << std::endl;
				os << std::setw(18) << "subchunk1_id: " << subchunk1_id << std::endl;
				os << std::setw(18) << "audio_format: " << _header.audio_format << std::endl;
				os << std::setw(18) << "num_channels: " << _header.num_channels << std::endl;
				os << std::setw(18) << "sample_rate: " << _header.sample_rate << std::endl;
				os << std::setw(18) << "byte_rate: " << _header.byte_rate << std::endl;
				os << std::setw(18) << "block_align: " << _header.block_align << std::endl;
				os << std::setw(18) << "bits_per_sample: " << _header.bits_per_sample << std::endl;

				os << std::setw(24) << "\n- DATA sub-chunk - " << std::endl;
				os << std::setw(18) << "subchunk2_id: " << subchunk2_id << std::endl;
				os << std::setw(18) << "subchunk2_size: " << _header.subchunk2_size << std::endl;

				// length = (total_sample_bytes / bytes_per_sample) / sample_rate
				double length = (double)(_header.subchunk2_size) / (_header.bits_per_sample / 8) /
								(double)(_header.sample_rate);

				os << std::setw(18) << "\nLength: " << length << "s" << std::endl;
			}

			std::ostream& print_data(std::ostream& os,
									 const std::function<std::ostream&(std::ostream&)>& format,
									 size_t amount = 0,
									 size_t from = 0) {
				format(os);

				for (int i = from; i < amount; i++)
					os << _data[i] << " ";

				return os << std::endl;
			}

			// Demo functionality
		public:
			void normalize() {
				float amplification_factor = 1 / maximum_amplitude();
				normalize(amplification_factor);
			}

			short maximum_intensity() {
				short max = _data[0];

				for (const short& sample : _data)
					if (std::abs(sample) > std::abs(max))
						max = sample;

				return max;
			}
			float maximum_amplitude() { return (float)maximum_intensity() / (INT16_MAX - 1); }

		private:
			void normalize(float factor) {
				demo::filters::Gain<float, short> normalizator = demo::filters::Gain<float, short>(factor);

				std::cout << factor << std::endl;

				for (short& sample : _data)
					sample = normalizator(sample);
			}
			size_t init_header(std::ifstream& file) {
				// TODO: implement skipping chunks not identified as id == 'fmt' || 'data'

				char chunk_id[4], format[4], subchunk1_id[4], subchunk2_id[4];
				size_t bytes_read;

				// Set cursor to the header start
				file.seekg(std::ios::beg);

				// Read the RIFF chunk descriptor
				file.read(chunk_id, 4);
				file.read((char*)&_header.chunk_size, 4);
				file.read(format, 4);
				bytes_read = 12;

				// Read the 'fmt' subchunk:
				file.read(subchunk1_id, 4);
				file.read((char*)&_header.subchunk1_size, 4);
				file.read((char*)&_header.audio_format, 2);
				file.read((char*)&_header.num_channels, 2);
				file.read((char*)&_header.sample_rate, 4);
				file.read((char*)&_header.byte_rate, 4);
				file.read((char*)&_header.block_align, 2);
				file.read((char*)&_header.bits_per_sample, 2);
				bytes_read += 24;

				// Read the 'data' subchunk:
				file.read(subchunk2_id, 4);
				file.read((char*)&_header.subchunk2_size, 4);
				bytes_read += 8;

				// Initialize our std::arrays with previously-initialized char[]'s
				for (short i = 0; i < 4; i++) {
					_header.chunk_id[i] = chunk_id[i];
					_header.format[i] = format[i];
					_header.subchunk1_id[i] = subchunk1_id[i];
					_header.subchunk2_id[i] = subchunk2_id[i];
				}

				return bytes_read;
			}

			size_t init_data(std::ifstream& file) {
				// Seek to the beginning of the (actual) information sector
				// Refer to "wav_header_format.jpg" for additional information
				file.seekg(44, std::ios_base::beg);

				// Initialize our samples vector
				short sample;
				size_t bytes_read = 0;
				size_t bytes_per_sample = _header.bits_per_sample / 8;

				// num_data_bytes / bytes_per_sample * sample_size -- this works idk lol
				for (int i = 0; i < _header.subchunk2_size / bytes_per_sample * 2; i += bytes_per_sample) {
					// Read one sample (2 bytes for 16bpp)
					file.read((char*)&sample, _header.bits_per_sample / 8);
					bytes_read += _header.bits_per_sample / 8;

					_data.push_back(sample);
				}

				return bytes_read;
			}

			int write_header(std::ofstream& file) {
				try {
					// Write the RIFF chunk descriptor
					file.write((char*)&_header.chunk_id, 4);
					file.write((char*)&_header.chunk_size, 4);
					file.write((char*)&_header.format, 4);

					// Write the 'fmt' sub-chunk
					file.write((char*)&_header.subchunk1_id, 4);
					file.write((char*)&_header.subchunk1_size, 4);
					file.write((char*)&_header.audio_format, 2);
					file.write((char*)&_header.num_channels, 2);
					file.write((char*)&_header.sample_rate, 4);
					file.write((char*)&_header.byte_rate, 4);
					file.write((char*)&_header.block_align, 2);
					file.write((char*)&_header.bits_per_sample, 2);

					// Write the 'data' sub-chunk
					file.write((char*)&_header.subchunk2_id, 4);
					file.write((char*)&_header.subchunk2_size, 4);
				} catch (...) {
					return FAILURE;
				}

				return SUCCESS;
			}

			int write_data(std::ofstream& file) {
				// Seek to the beginning of the (actual) information sector
				// Refer to "wav_header_format.jpg" for additional information
				file.seekp(0x2C, std::ios_base::beg);

				size_t bytes_written = 0;
				int bytes_per_sample = _header.bits_per_sample / 8;

				for (const short& elem : _data)
					try {
						file.write((char*)&elem, bytes_per_sample);
						bytes_written += bytes_per_sample;
					} catch (...) {
						return FAILURE;
					}

				// std::cout << "Written " << bytes_written << " bytes." << std::endl;

				return SUCCESS;
			}
	};

} // namespace wav

#endif