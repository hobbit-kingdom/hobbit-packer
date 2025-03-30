#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <string>
#include <map>

namespace fs = std::filesystem;

// Helper function to convert a string to lowercase
std::string toLower(const std::string& str) {
	std::string lowerStr = str;
	std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
	return lowerStr;
}

void runUndfsCommand(const fs::path& filePath) {
	// Create the command to run the 'undfs.exe' on the file
	std::string command = "undfs.exe \"" + filePath.string() + "\"";

	// Execute the command
	int result = std::system(command.c_str());

	// Check if the command ran successfully
	if (result != 0) {
		std::cerr << "Error running command for file: " << filePath << std::endl;
	}
	else {
		std::cout << "Successfully executed command for: " << filePath << std::endl;
	}
}

void deleteFilesInFolder(bool keepAudioFiles) {
	fs::path currentPath = fs::current_path();

	try {
		// Iterate through all files in the directory
		for (const auto& entry : fs::directory_iterator(currentPath)) {
			const auto& filePath = entry.path();
			std::string fileExt = toLower(filePath.extension().string());

			// If we should keep audio files, don't delete them
			if (keepAudioFiles) {
				// Check if the file is one of the audio files and skip deletion
				std::string fileName = toLower(filePath.filename().string());
				if (fileName == "audio1.dfs" || fileName == "audio2.dfs" ||
					fileName == "audio1.000" || fileName == "audio2.000") {
					std::cout << "Keeping file: " << filePath << std::endl;
					continue;
				}
			}

			// Check if the file ends with .dfs or .000 (case insensitive)
			if (fileExt == ".dfs" || fileExt == ".000") {
				// Delete the file
				std::cout << "Deleting file: " << filePath << std::endl;
				fs::remove(filePath);
			}
		}
	}
	catch (const fs::filesystem_error& e) {
		std::cerr << "Filesystem error: " << e.what() << std::endl;
	}
}

void unpackFiles(bool skipAudioFiles) {
	fs::path currentPath = fs::current_path();

	try {
		// Iterate through all files in the current directory
		for (const auto& entry : fs::directory_iterator(currentPath)) {
			const auto& filePath = entry.path();
			std::string fileName = toLower(filePath.filename().string());

			// Skip audio1.dfs and audio2.dfs files if we are skipping them
			if (skipAudioFiles && (fileName == "audio1.dfs" || fileName == "audio2.dfs")) {
				std::cout << "Skipping file: " << filePath.filename() << std::endl;
				continue; // Skip these files
			}

			// Check if the file has a .dfs extension (case insensitive)
			std::string fileExt = toLower(filePath.extension().string());
			if (fileExt == ".dfs") {
				runUndfsCommand(filePath);
			}
		}

		// After processing .dfs files, delete all .dfs and .000 files
		deleteFilesInFolder(skipAudioFiles);
	}
	catch (const fs::filesystem_error& e) {
		std::cerr << "Filesystem error: " << e.what() << std::endl;
	}
}

// Mapping for level names to special names
std::map<std::string, std::string> levelMap = {
	{"CH00_DREAMWORLD", "Ch00_Dre"},
	{"CH01_HOBBITON", "Ch01_Hob"},
	{"CH02_ROASTMUTTON", "Ch02_Roa"},
	{"CH02A_TROLLHOLE", "Ch02a_Tr"},
	{"CH4_OVERHILL", "Ch4_Over"},
	{"CH05_SWORDLIGHT", "Ch05_Swo"},
	{"CH07_BARRELSOUTOFBOND", "Ch07_Bar"},
	{"CH08_LAKETOWN", "Ch08_Lak"},
	{"CH09_SMAUG", "Ch09_Sma"},
	{"CH10_LONELY_MOUNTAIN", "Ch10_Lon"},
	{"CH11_CLOUDSBURST", "Ch11_Clo"},
	{"MIRKWOOD", "Mirkwood"}
};

// Function to check if a folder exists
bool folderExists(const fs::path& folderPath) {
	return fs::exists(folderPath) && fs::is_directory(folderPath);
}

// Function to move level folders based on the mapping
void moveLevelFolders() {
	fs::path currentPath = fs::current_path(); // Get the current directory

	// Define the levels directory
	fs::path levelsDir = currentPath / "levels";

	if (!folderExists(levelsDir)) {
		std::cerr << "Error: 'levels' folder not found in the current directory!" << std::endl;
		return;
	}

	try {
		// Iterate over all folders in the 'levels' directory
		for (const auto& entry : fs::directory_iterator(levelsDir)) {
			if (fs::is_directory(entry)) {
				std::string levelFolderName = entry.path().filename().string();

				// Check if the level folder name exists in the map
				auto it = levelMap.find(levelFolderName);
				if (it != levelMap.end()) {
					// Get the special name for the folder
					std::string specialName = it->second;

					// Create the new folder structure (special_name/LEVELS)
					fs::path newFolderPath = currentPath / specialName / "LEVELS";
					fs::create_directories(newFolderPath); // Create the necessary directories

					// Move the level folder into the new folder structure
					fs::rename(entry.path(), newFolderPath / entry.path().filename());

					std::cout << "Moved " << levelFolderName << " to " << newFolderPath << std::endl;

					// Now apply the batch file to the special_name folder itself (not inside LEVELS)
					std::string command = "pack_dfs_Drag'n'Drop.bat \"" + (currentPath / specialName).string() + "\"";
					std::cout << "Running command: " << command << std::endl;
					int result = std::system(command.c_str());

					// Check if the command ran successfully
					if (result != 0) {
						std::cerr << "Error running batch file for: " << levelFolderName << std::endl;
					}

					// Delete the 'special_name' folder after processing
					if (fs::exists(currentPath / specialName)) {
						fs::remove_all(currentPath / specialName); // This will delete the folder and all its contents
						std::cout << "Deleted folder: " << specialName << std::endl;
					}
				}
				else {
					std::cout << "No mapping found for level: " << levelFolderName << ", skipping..." << std::endl;
				}
			}
		}

		// Delete the 'LEVELS' folder after processing
		fs::path levelsFolderPath = currentPath / "LEVELS";
		if (fs::exists(levelsFolderPath)) {
			fs::remove_all(levelsFolderPath); // Delete the 'LEVELS' folder and its contents
			std::cout << "Deleted 'LEVELS' folder: " << levelsFolderPath << std::endl;
		}
	}
	catch (const fs::filesystem_error& e) {
		std::cerr << "Filesystem error: " << e.what() << std::endl;
	}
}



int main() {
	int choice = 0;
	char skipAudioChoice = 'n';

	std::cout << "Choose an option:\n";
	std::cout << "1. Pack\n";
	std::cout << "2. Unpack (Run undfs.exe and delete .dfs and .000 files)\n";
	std::cout << "Enter your choice (1 or 2): ";
	std::cin >> choice;

	if (choice == 2) {
		std::cout << "Do you want to skip audio1.dfs and audio2.dfs files during unpacking? (y/n): ";
		std::cin >> skipAudioChoice;

		bool skipAudioFiles = (skipAudioChoice == 'y' || skipAudioChoice == 'Y');

		if (skipAudioFiles) std::cout << "Will skip audio files\n"; // Confirmation that audio files will be skipped

		std::cout << "You chose to unpack. Running the unpacking process...\n";
		unpackFiles(skipAudioFiles);
	}
	else if (choice == 1) {
		std::cout << "You chose to pack...\n";
		moveLevelFolders();
	}
	else {
		std::cerr << "Invalid choice! Please choose 1 or 2.\n";
	}

	// Add prompt to press any key to continue
	std::cout << "\nPress any key to continue...";
	std::cin.get();  // To catch the leftover newline character
	std::cin.get();  // Wait for user input

	return 0;
}
