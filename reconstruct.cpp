#include <iostream>
#include <fstream>
#include <sstream>
#include <deque>
#include <string>
#include <ctime>
#include <iomanip>

//Grace & Anne's reconstruction program
//sources:
//Parsing time : https://www.geeksforgeeks.org/date-and-time-parsing-in-cpp/
//there's a lot of commented out output statements that we used for testing

//function that parses a timestamp string into std::time_t
std::time_t parseTimestamp(const std::string& timestamp) {
    std::tm tm = {};
    std::istringstream ss(timestamp);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return std::mktime(&tm);
}

//reconstruct a file from a journal up to a given timestamp
void reconstructFile(const std::string& journalPath, const std::string& outputPath, const std::string& targetTimestamp) {
    std::ifstream journal(journalPath);
    std::cout << "opened " << journalPath << std::endl;
    if (!journal.is_open()) {
        std::cerr << "Could not open journal file: " << journalPath << std::endl;
        return;
    }

    std::deque<std::string> fileState;  //represents the file's reconstructed content
    std::time_t targetTime = parseTimestamp(targetTimestamp);

    std::string line;
    while (std::getline(journal, line)) {
        // Parse the journal entry
        std::istringstream iss(line);
        std::string timestamp, action, content;
        int lineNumber;
        std::cout << "Parsing journal entry: " << line << std::endl;

        timestamp = line.substr(0, 20);

        std::time_t entryTime = parseTimestamp(timestamp);
        //std::cout << "Parsed timestamp: " << timestamp << " (" << std::put_time(std::localtime(&entryTime), "%Y-%m-%d %H:%M:%S") << "), comparing with target: " << targetTimestamp << std::endl;

        if (entryTime > targetTime) break;
        
        // Extract action (+ or -) and content (very hard coded)
        char actionChar;
        std::istringstream entry(line);
        entry.ignore(22); //skips to line number
        entry >> lineNumber;
        entry.ignore(1);  //skips to action character
        entry >> actionChar;                                 
        entry.ignore(3);     //skips to content!                                
        std::getline(entry, content);
        
        //std::cout << "Action: " << actionChar << ", Line Number: " << lineNumber << ", Content: " << content << std::endl;
       
       if (actionChar == '+') {
            if (lineNumber > fileState.size()) {
                fileState.resize(lineNumber); // Expand the file if necessary
            }
            fileState[lineNumber - 1] = content;  // Insert or overwrite the line
        } else if (actionChar == '-') {
            if (lineNumber <= fileState.size()) {
                fileState[lineNumber - 1] = ""; // Mark the line as deleted
            }
        }
    }

    journal.close();
    std::ofstream outputFile(outputPath);

    //std::cout << "Writing reconstructed file to: " << outputPath << std::endl;
    for (const auto& contentLine : fileState) {
        if (!contentLine.empty()) {
            outputFile << contentLine << std::endl;
        }
    }

    outputFile.close();
    //std::cout << "File reconstructed and saved to: " << outputPath << std::endl;
}


int main() {
    std::string journalPath, outputPath, targetTimestamp;
    std::cout<< "Anne and Grace's Reconstruction program!" <<std::endl;
    
    std::cout<< "Enter the journal path: ";
    std::cin >> journalPath;
    //journalPath = "./journal/eggs62.DAT";
    
    std::cout << "Enter the output path: ";
    std::cin >> outputPath;
    //outputPath = "./reconstructed/outputfile.txt";
    
    std::cout << "Enter the Timestamp (YYYY-MM-DD HH:MM:SS): "; 
    std::cin.ignore();
    std::getline(std::cin, targetTimestamp);
    //targetTimestamp = "2024-12-09 16:48:24";
    
    reconstructFile(journalPath, outputPath, targetTimestamp);

    return 0;
}