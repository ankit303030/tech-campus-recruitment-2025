
---

## src/extract_logs.cpp

Below is the **C++ implementation** of the index-based solution. It handles two commands:

1. **`--build-index logFilePath indexFilePath`**  
   Builds the index file (`indexFilePath`) from the log file (`logFilePath`).
2. **`--date YYYY-MM-DD logFilePath indexFilePath`**  
   Uses the pre-built index file to quickly extract logs for the given date, writing them to `output/output_YYYY-MM-DD.txt`.

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <filesystem> // C++17 for create_directories
#include <cstdlib>    // for EXIT_SUCCESS, EXIT_FAILURE

// Forward declarations
void buildIndex(const std::string &logFilePath, const std::string &indexFilePath);
void extractLogsForDate(const std::string &date, const std::string &logFilePath, const std::string &indexFilePath);

int main(int argc, char* argv[])
{
    // Usage examples:
    // Build index:  ./extract_logs --build-index test_logs.log log_index.txt
    // Extract logs: ./extract_logs --date 2024-12-01 test_logs.log log_index.txt

    if (argc < 2)
    {
        std::cerr << "No arguments provided. Use --build-index or --date.\n";
        return EXIT_FAILURE;
    }

    std::string command = argv[1];
    if (command == "--build-index")
    {
        if (argc < 4)
        {
            std::cerr << "Usage: ./extract_logs --build-index <logFile> <indexFile>\n";
            return EXIT_FAILURE;
        }
        std::string logFilePath   = argv[2];
        std::string indexFilePath = argv[3];

        buildIndex(logFilePath, indexFilePath);
    }
    else if (command == "--date")
    {
        if (argc < 5)
        {
            std::cerr << "Usage: ./extract_logs --date <YYYY-MM-DD> <logFile> <indexFile>\n";
            return EXIT_FAILURE;
        }
        std::string date          = argv[2];
        std::string logFilePath   = argv[3];
        std::string indexFilePath = argv[4];

        extractLogsForDate(date, logFilePath, indexFilePath);
    }
    else
    {
        std::cerr << "Invalid command. Use --build-index or --date.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void buildIndex(const std::string &logFilePath, const std::string &indexFilePath)
{
    std::cout << "[INFO] Building index from " << logFilePath << "..." << std::endl;

    std::ifstream logFile(logFilePath, std::ios::in | std::ios::binary);
    if (!logFile.is_open())
    {
        std::cerr << "[ERROR] Could not open log file: " << logFilePath << std::endl;
        return;
    }

    std::ofstream indexFile(indexFilePath);
    if (!indexFile.is_open())
    {
        std::cerr << "[ERROR] Could not open index file for writing: " << indexFilePath << std::endl;
        return;
    }

    std::string currentDate;
    std::streampos offset;

    while (!logFile.eof())
    {
        offset = logFile.tellg();

        std::string line;
        if (!std::getline(logFile, line))
        {
            break; // Reached EOF or encountered a read error
        }

        // Each log line starts with "YYYY-MM-DD HH:MM:SS"
        // Extract the first 10 characters for the date
        if (line.size() >= 10)
        {
            std::string datePart = line.substr(0, 10);
            if (datePart != currentDate)
            {
                currentDate = datePart;
                // Write (date, file_offset) to index
                indexFile << currentDate << "," << offset << "\n";
            }
        }
    }

    logFile.close();
    indexFile.close();

    std::cout << "[INFO] Index built successfully -> " << indexFilePath << std::endl;
}

void extractLogsForDate(const std::string &date, const std::string &logFilePath, const std::string &indexFilePath)
{
    std::cout << "[INFO] Extracting logs for " << date << "..." << std::endl;

    // Read the index file into memory
    // Each line: "date,offset"
    std::vector<std::pair<std::string, std::streampos>> dateOffsets;
    {
        std::ifstream idxFile(indexFilePath);
        if (!idxFile.is_open())
        {
            std::cerr << "[ERROR] Could not open index file: " << indexFilePath << std::endl;
            return;
        }

        std::string line;
        while (std::getline(idxFile, line))
        {
            std::stringstream ss(line);
            std::string d, offsetStr;
            if (std::getline(ss, d, ',') && std::getline(ss, offsetStr))
            {
                std::streampos pos = static_cast<std::streampos>(std::stoll(offsetStr));
                dateOffsets.emplace_back(d, pos);
            }
        }
    }

    // Ensure sorted by offset (might already be in order, but just to be safe)
    std::sort(dateOffsets.begin(), dateOffsets.end(),
              [](auto &a, auto &b) {
                  return a.second < b.second;
              });

    // Find the start and end offsets for the requested date
    std::streampos startOffset = -1;
    std::streampos endOffset   = -1;

    for (size_t i = 0; i < dateOffsets.size(); ++i)
    {
        if (dateOffsets[i].first == date)
        {
            startOffset = dateOffsets[i].second;
            if (i + 1 < dateOffsets.size())
            {
                endOffset = dateOffsets[i + 1].second;
            }
            break;
        }
    }

    if (startOffset == -1)
    {
        std::cout << "[INFO] No logs found for date: " << date << std::endl;
        return;
    }

    // Prepare the output directory
    std::filesystem::create_directories("output");
    std::string outputFilePath = "output/output_" + date + ".txt";

    std::ifstream logFile(logFilePath, std::ios::in | std::ios::binary);
    if (!logFile.is_open())
    {
        std::cerr << "[ERROR] Failed to open log file again: " << logFilePath << std::endl;
        return;
    }

    std::ofstream outFile(outputFilePath);
    if (!outFile.is_open())
    {
        std::cerr << "[ERROR] Could not create output file: " << outputFilePath << std::endl;
        return;
    }

    // Seek to the start offset
    logFile.seekg(startOffset);

    while (!logFile.eof())
    {
        // If we have a valid end offset, stop once we pass it
        if (endOffset != -1 && logFile.tellg() >= endOffset)
        {
            break;
        }

        std::string line;
        if (!std::getline(logFile, line))
        {
            break; // EOF or error
        }

        // If the date changes, stop
        if (line.size() >= 10)
        {
            if (line.substr(0, 10) != date)
            {
                break;
            }
        }

        // Write the matched log line
        outFile << line << "\n";
    }

    logFile.close();
    outFile.close();

    std::cout << "[INFO] Logs for " << date << " written to " << outputFilePath << std::endl;
}
