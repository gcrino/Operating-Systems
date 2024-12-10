#include <iostream>
#include <fstream>
#include <sys/inotify.h>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <unordered_map>
#include <ctime>
#include <filesystem>
#include <deque>
/*Anne and Grace's Term Project
sources:
inotify - https://man7.org/linux/man-pages/man7/inotify.7.html
inotify setup - https://www.thegeekstuff.com/2010/04/inotify-c-program-example/
deque -	 https://cplusplus.com/reference/deque/deque/ (replaced vectors for us)
filesystem - https://en.cppreference.com/w/cpp/filesystem
filesystem (found copy overwrite here) - https://en.cppreference.com/w/cpp/filesystem/copy_options
unordered_map - https://cplusplus.com/reference/unordered_map/
ctime - https://cplusplus.com/reference/ctime/
*/

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))

namespace fs = std::filesystem;

//reads file into a deque
std::deque<std::string> readFile(const std::string& path) {
	std::deque<std::string> lines;
	std::ifstream file(path);
	std::string line;
	while (std::getline(file, line)) {
		lines.push_back(line);
	}
	return lines;
}

//keeps the journal file length at 50
void checkJournalLength(const std::string& path) {
	std::deque<std::string> lines;
	std::ifstream file(path);
	std::string line;
	while (std::getline(file, line)) {
		lines.push_back(line);
	}
	while (lines.size() > 50) {
		lines.pop_front();
	}
	std::ofstream journal(path, std::ios::trunc);
	for (const auto& entry : lines) {
		journal << entry << std::endl;
	}
}

//writes an entry to the journal file
void writeJournal(const std::string& path, const std::string& line, int num, char action) {
	std::ofstream journal(path, std::ios::app);
	std::cout << "Writing to journal: " << path << std::endl;
	std::time_t current = std::time(nullptr);
	char timestamp[20];
	std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&current));
	journal << timestamp << ", l" << num << " " << action << " : " << line << std::endl;
	journal.close();
	checkJournalLength(path);
}

int main() {
	std::unordered_map<std::string, std::string> fileToJournalMap;
	int fd = inotify_init(); 
	int wd = inotify_add_watch(fd, "./watched", IN_CREATE | IN_MODIFY | IN_DELETE);
	int length, i;
	std::cout << "Now watching folder ./watched" << std::endl;

	char buffer[BUF_LEN];

	while (true) {
		length = read(fd, buffer, BUF_LEN);

		i = 0;
		while (i < length) {
			struct inotify_event* event = (struct inotify_event*)&buffer[i];
			std::string fileName(event->name);
			std::string watchedPath = "./watched/" + fileName;
			std::string copyPath = "./.hidden/" + fileName;

			if (event->len) {
				if (event->mask & IN_CREATE) {
					if (fileToJournalMap.find(fileName) == fileToJournalMap.end()) {
						//creates a new journal file
						std::string journalPath = "./journal/" + fileName + "_" + std::to_string(std::time(nullptr)) + ".DAT";
						//unordered map that links a watched file to a journal file
						fileToJournalMap[fileName] = journalPath;
						std::cout << "Created new journal for: " << fileName << std::endl;

						//creates initial copy of the file (for +/- comparison purposes) (basically in-house diff)
						if (fs::exists(watchedPath)) {
							fs::copy_file(watchedPath, copyPath, fs::copy_options::overwrite_existing);
						}
					}
				}
				//if a file is modified
				if (event->mask & IN_MODIFY) {
					if (fileToJournalMap.find(fileName) != fileToJournalMap.end()) {
						//makes the two files into deques of strings, with each line in the queue
						std::deque<std::string> copyContent = readFile(copyPath);
						std::deque<std::string> watchedContent = readFile(watchedPath);
						char action;
						//compares log changes (+ or -) (start of in-house diff)
						int num = 1;
						//iterates through the longer of the two files
						for (int j = 0; j < std::max(copyContent.size(), watchedContent.size()); j++) {
							//if the line is in the watched file but isn't in the copy or is different from the copy
							if (j < watchedContent.size() && 
								(j >= copyContent.size() || watchedContent[j] != copyContent[j])) { 
								//determines if the line is different
								if (j < copyContent.size()) {
									action = '-';
								}
								else {
									action = '+';
								}
								writeJournal(fileToJournalMap[fileName], watchedContent[j], num, action);
							} 
							//if the line exists in the copy but not the watched (- cause we lost it)
							else if (j < copyContent.size() && j >= watchedContent.size()) {
								writeJournal(fileToJournalMap[fileName], copyContent[j], num, '-');
							}
							num++;
						}

						//updates the copy 
						fs::copy_file(watchedPath, copyPath, fs::copy_options::overwrite_existing);
					}
				}
				//if a file is deleted
				if (event->mask & IN_DELETE) {
					if (fileToJournalMap.find(fileName) != fileToJournalMap.end()) {
						std::cout << "File deleted: " << fileName << std::endl;
						//takes out the file from the map, so if another one w/a new name occurs then it won't overlap
						fileToJournalMap.erase(fileName);
					}
				}
			}

			i += EVENT_SIZE + event->len;
		}
	}

	inotify_rm_watch(fd, wd);
	close(fd);
	return 0;
}