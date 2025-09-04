#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <stdexcept>

namespace IniParse {

    using Section = std::map<std::string, std::string>;
    using IniData = std::map<std::string, Section>;

    // Utility function to trim whitespace from both ends of a string
    std::string trim(const std::string& str)
    {
        size_t first = str.find_first_not_of(" \t");
        if (first == std::string::npos)
            return "";
        size_t last = str.find_last_not_of(" \t");
        return str.substr(first, last - first + 1);
    }

    // Function to parse an INI file
    IniData parseIni(const std::string& filepath)
    {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filepath);
        }

        std::string line;
        std::string currentSection = "DEFAULT";  // Handle keys before any section
        IniData iniData;

        while (std::getline(file, line))
        {
            line = trim(line);
            // Skip comments and empty lines
            if (line.empty() || line[0] == ';' || line[0] == '#')
                continue;

            // Handle section headers - fix malformed section check
            if (line[0] == '[') {
                if (line.back() != ']') {
                    continue; // Skip malformed section headers
                }
                currentSection = line.substr(1, line.size() - 2);
            }
            else
            {
                // Handle key-value pairs
                size_t equalPos = line.find('=');
                if (equalPos != std::string::npos)
                {
                    std::string key = trim(line.substr(0, equalPos));
                    std::string value = trim(line.substr(equalPos + 1));
                    iniData[currentSection][key] = value;
                }
            }
        }

        return iniData;
    }

} // namespace IniParse

