// Copyright (c) 2014 Nezametdinov E. Ildus
// See LICENSE.TXT for licensing details

#include "Salsa20.h"

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>

using namespace salsa20;

/**
 * Represents program.
 */
class Program
{
public:
        Program(): inputFileName_(), outputFileName_(), shouldShowHelp_(false)
        {
                std::memset(key_, 0, sizeof(key_));
        }
        Program(const Program&) = delete;
        Program(Program&&) = delete;
        ~Program() {}
        Program& operator =(const Program&) = delete;
        Program& operator =(Program&&) = delete;

        /**
         * \brief Reads parameters from command line and validates them.
         * \param[in] argc number of command line arguments passed
         * \param[in] argv array of command line arguments
         * \return true on success
         */
        bool initialize(int argc, char* argv[])
        {
                std::string key;
                shouldShowHelp_ = false;

                for(int i = 0; i < argc; ++i)
                {
                        std::string parameter = argv[i];

                        if(parameter == "-p")
                        {
                                if((argc - i - 1) != 3)
                                        break;

                                inputFileName_ = argv[++i];
                                outputFileName_ = argv[++i];
                                key = argv[++i];
                                break;
                        }

                        if(parameter == "-h")
                        {
                                shouldShowHelp_ = true;
                                return true;
                        }
                }

                if(inputFileName_.empty())
                {
                        std::cout << "E: Input file name was not specified." << std::endl;
                        return false;
                }

                if(outputFileName_.empty())
                {
                        std::cout << "E: Output file name was not specified." << std::endl;
                        return false;
                }

                if(inputFileName_ == outputFileName_)
                {
                        std::cout << "E: Input and output files should be distinct." << std::endl;
                        return false;
                }

                if(key.empty())
                {
                        std::cout << "E: Key was not specified." << std::endl;
                        return false;
                }

                if(!readKeyFromString(key))
                {
                        std::cout << "E: Invalid key value." << std::endl;
                        return false;
                }

                return true;
        }

        /**
         * \brief Encrypts or decrypts the file.
         * \return true on success
         */
        bool execute()
        {
                if(shouldShowHelp_)
                {
                        std::cout << "Usage: salsa20 -p INPUT OUTPUT KEY" << std::endl;
                        std::cout << "       salsa20 -h" << std::endl;
                        std::cout << std::endl << "Salsa20 is a stream cypher (see http://cr.yp.to/snuffle.html).";
                        std::cout << std::endl << std::endl;
                        std::cout << "Options:" << std::endl;
                        std::cout << "  -h Shows this help text." << std::endl;
                        std::cout << "  -p Encrypts or decrypts file INPUT with KEY and outputs result to file OUTPUT.";
                        std::cout << std::endl;
                        std::cout << "     KEY is a 32-byte key concatenated with 8-byte IV written in HEX.";
                        std::cout << std::endl;
                        return true;
                }

                std::ifstream inputStream(inputFileName_, std::ios_base::binary);
                if(!inputStream)
                {
                        std::cout << "E: Could not open input file." << std::endl;
                        return false;
                }

                std::ofstream outputStream(outputFileName_, std::ios_base::binary);
                if(!outputStream)
                {
                        std::cout << "E: Could not create output file." << std::endl;
                        return false;
                }

                const size_t chunkSize = NUM_OF_BLOCKS_PER_CHUNK * Cypher::BLOCK_SIZE;
                uint8_t chunk[chunkSize];

                // determine size of the file
                inputStream.seekg(0, std::ios_base::end);
                size_t fileSize = inputStream.tellg();
                inputStream.seekg(0, std::ios_base::beg);

                // compute number of chunks and size of the remainder
                size_t numChunks = fileSize / chunkSize;
                size_t remainderSize = fileSize % chunkSize;

                // process file
                Cypher cypher(key_);
                cypher.setIv(&key_[IV_OFFSET]);
                std::cout << "Processing file \"" << inputFileName_ << '"' << std::endl;

                for(size_t i = 0; i < numChunks; ++i)
                {
                        inputStream.read(reinterpret_cast<char*>(chunk), sizeof(chunk));
                        cypher.processBlocks(chunk, chunk, NUM_OF_BLOCKS_PER_CHUNK);
                        outputStream.write(reinterpret_cast<const char*>(chunk), sizeof(chunk));

                        float percentage = 100.0f * static_cast<float>(i + 1) / static_cast<float>(numChunks);
                        std::printf("[%3.2f]\r", percentage);
                }

                if(remainderSize != 0)
                {
                        inputStream.read(reinterpret_cast<char*>(chunk), remainderSize);
                        cypher.processBytes(chunk, chunk, remainderSize);
                        outputStream.write(reinterpret_cast<const char*>(chunk), remainderSize);
                        std::cout << "[100.00]";
                }

                std::cout << std::endl << "OK" << std::endl;
                return true;
        }

private:
        /// Helper constants
        enum
        {
                NUM_OF_BLOCKS_PER_CHUNK = 8192,
                IV_OFFSET = Cypher::KEY_SIZE,
                KEY_SIZE  = Cypher::KEY_SIZE + Cypher::IV_SIZE
        };

        std::string inputFileName_, outputFileName_;
        uint8_t key_[KEY_SIZE];
        bool shouldShowHelp_;

        /**
         * \brief Reads byte from string.
         * \param[in] string string
         * \param[out] byte byte
         * \return true on success
         */
        bool readByte(const char* string, uint8_t& byte)
        {
                byte = 0;

                for(size_t i = 0; i < 2; ++i)
                {
                        uint8_t value = 0;
                        char c = string[i];

                        if(c >= '0' && c <= '9')
                                value = c - '0';
                        else if(c >= 'A' && c <= 'F')
                                value = c - 'A' + 0x0A;
                        else if(c >= 'a' && c <= 'f')
                                value = c - 'a' + 0x0A;
                        else
                                return false;

                        byte |= (value << (4 - i * 4));
                }

                return true;
        }

        /**
         * \brief Reads key from string.
         * \param[in] string string
         * \return true on success
         */
        bool readKeyFromString(const std::string& string)
        {
                if(string.length() != 2 * KEY_SIZE)
                        return false;

                for(size_t i = 0; i < string.length(); i += 2)
                {
                        if(!readByte(&string[i], key_[i / 2]))
                                return false;
                }

                return true;
        }

};

// Entry point
int main(int argc, char* argv[])
{
        Program program;

        if(!program.initialize(argc, argv))
                return 1;

        if(!program.execute())
                return 2;

        return 0;
}
