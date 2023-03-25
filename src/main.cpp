/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Module:       main.cpp                                                  */
/*    Author:       LemLib Team                                               */
/*    Created:      3/23/2023, 12:18:22 PM                                    */
/*    Description:  LemLib file system serial interpreter                     */
/*                                                                            */
/*----------------------------------------------------------------------------*/
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string.h>

// Exception codes:
// VFS_INIT_FAILED
// FILE_NOT_FOUND
// FILE_ALREADY_EXISTS
// CANNOT_OPEN_FILE

/**
 * @brief Convert a value to a string
 *
 * @tparam T
 * @param value value to convert
 * @return std::string
 */
template <typename T> std::string to_string(T value) {
    std::ostringstream os;
    os << value;
    return os.str();
}

typedef struct VFS_INIT_FAILED {};
VFS_INIT_FAILED vfsInitFailed;

typedef struct FILE_NOT_FOUND {};
FILE_NOT_FOUND fileNotFound;

typedef struct FILE_ALREADY_EXISTS {};
FILE_ALREADY_EXISTS fileAlreadyExists;

typedef struct CANNOT_OPEN_FILE {};
CANNOT_OPEN_FILE cannotOpenFile;

/**
 * @brief Structure for an entry in the index file
 *
 * @param name the name of the file
 * @param sector the sector the file is stored in
 */
typedef struct lemlibFile {
        std::string name;
        std::string sector;
} lemlibFile;

/**
 * @brief Initialize the file system
 *
 */
void initVFS() {
    // Check if the index file exists
    std::ifstream indexFile;
    indexFile.open("index.txt");
    // If the index file does not exist, create it
    if (!indexFile.is_open()) {
        std::ofstream indexFile;
        indexFile.open("index.txt");
        // throw an exception if the index file could not be created
        if (!indexFile.is_open()) throw vfsInitFailed;
        indexFile.close();
    }
}

/**
 * @brief Read the index file
 *
 * @return std::vector<lemlibFile> contents of the index file
 */
std::vector<lemlibFile> readFileIndex() {
    // Initialize the vector
    std::vector<lemlibFile> index;
    // Open the index file
    std::ifstream indexFile;
    indexFile.open("index.txt");
    // throw an exception if the index file could not be opened
    if (!indexFile.is_open()) throw cannotOpenFile;
    // iterate through the index file
    for (std::string line; std::getline(indexFile, line);) {
        // split the line into the name and the sector
        // the number after the last backslash is the sector
        std::string name = line.substr(0, line.find_last_of("/"));
        std::string sector = line.substr(line.find_last_of("/") + 1);
        // add the file to the index
        index.push_back({name, sector});
    }
    return index;
}

/**
 * @brief Get the Sector object
 *
 * @param path the virtual of the virtual file
 * @return const char* the sector the file is stored in, or null if the file is not found
 */
const char* getFileSector(const char* path) {
    // Read the index file
    std::vector<lemlibFile> index = readFileIndex();
    // Iterate through the index
    for (const lemlibFile& file : index) {
        // Check if the name matches
        if (file.name == path) return file.sector.c_str();
    }
    // Return null if the file is not found
    return NULL;
}

/**
 * @brief List all the files and folders in a directory
 *
 * @param dir the directory to list
 * @return std::vector <std::string> a vector of all the files and folders in the directory
 */
std::vector<std::string> listDirectory(const char* dir, bool recursive = false) {
    // Initialize the vector
    std::vector<std::string> files;
    // Read the index file
    std::vector<lemlibFile> index = readFileIndex();
    // Iterate through the index
    for (const lemlibFile& line : index) {
        // Check if the name starts with the directory
        if (line.name.find(dir) == std::string::npos) continue;
        // remove the directory from the name
        std::string name = line.name.substr(line.name.find(dir) + strlen(dir));
        // if there is a remaining slash, a directory is found
        if (name.find("/") != std::string::npos && !recursive) name = name.substr(0, name.find("/")) + "/";
        // push back the name, if it is not already in the vector
        bool found = false;
        for (const std::string& file : files) {
            if (file == name) {
                found = true;
                break;
            }
        }
        if (!found) files.push_back(name);
    }
    return files;
}

/**
 * @brief Check if a file exists
 *
 * @param path path of the file
 * @return true the file exists
 * @return false the file does not exist
 */
bool fileExists(const char* path) {
    // Read the index file
    std::vector<lemlibFile> index = readFileIndex();
    // Iterate through the index
    for (const lemlibFile& file : index) {
        // Check if the name matches
        if (file.name == path) return true;
    }
    return false;
}

/**
 * @brief delete a virtual file
 *
 * @param path the path of the virtual file
 */
void deleteFile(const char* path) {
    // check if the file exists
    if (!fileExists(path)) throw fileNotFound;
    // empty the sector the file is stored in
    std::ofstream sector;
    sector.open(getFileSector(path));
    sector << "";
    sector.close();
    // remove the file from the index file
    std::vector<lemlibFile> index = readFileIndex();
    std::ofstream indexFile;
    indexFile.open("index.txt");
    if (!indexFile.is_open()) throw cannotOpenFile;
    for (const lemlibFile& line : index) {
        if (line.name != path) indexFile << line.name << "/" << line.sector << std::endl;
    }
    indexFile.close();
}

/**
 * @brief Create a virtual file
 *
 * @param path the path of the virtual file
 * @return const char* the sector the file is stored in
 */
const char* createFile(const char* path, bool overwrite = true) {
    // Create the file in the index file
    std::ofstream indexFile;
    indexFile.open("index.txt", std::ios_base::app);
    if (!indexFile.is_open()) throw cannotOpenFile;
    // Check if the file already exists
    if (fileExists(path)) {
        // If the file should be overwritten, delete the file
        if (overwrite) deleteFile(path);
        // Otherwise, throw an exception
        else throw fileAlreadyExists;
    }
    // Find the first empty sector
    std::vector<lemlibFile> index = readFileIndex();
    int sector = 0;
    for (const lemlibFile& file : index) {
        if (file.sector == to_string(sector)) sector++;
    }
    // Create the file
    indexFile << path << "/" << sector << std::endl;
    indexFile.close();
    // create the sector file
    std::ofstream sectorFile;
    // check if the sector file was created
    if (!sectorFile.is_open()) throw cannotOpenFile;
    sectorFile.open(to_string(sector));
    sectorFile << "";
    sectorFile.close();
    return to_string(sector).c_str();
}

/**
 * @brief Write data to a virtual file
 *
 * @param path the path of the virtual file
 * @param data the data to write to the file, separated by \n
 * @return std::string the sector the file is stored in
 */
std::string write(const char* path, const char* data) {
    // Create the file if it does not exist
    if (!fileExists(path)) createFile(path);

    std::ofstream file;
    file.open(getFileSector(path));
    if (!file.is_open()) throw cannotOpenFile;
    std::string line;
    std::istringstream stream(data);
    while (std::getline(stream, line, '\n')) file << line << std::endl;
    file.close();

    return getFileSector(path);
}

/**
 * @brief Read data from a virtual file
 *
 * @param path the path of the virtual file
 *
 * @return std::string the data in the file, separated by \n
 */
std::string read(const char* path) {
    // Check if it exists
    if (!fileExists(path)) throw fileNotFound;

    // Find the file
    std::ifstream file;
    file.open(getFileSector(path));
    if (!file.is_open()) throw cannotOpenFile;
    // Read the contents, line by line
    std::string data = "";
    std::string line;
    while (std::getline(file, line)) data += line + "\n";
    file.close();

    return data;
}

/**
 * @brief Check if a path is a directory
 *
 * @param path the path to check
 *
 * @return true the path is a directory
 */
bool isDirectory(const char* path) { return path[strlen(path) - 1] == '/'; }

/**
 * @brief Initializes the listeners for the extension and
 * CLI to communicate with the virtual file system
 */
void initializeSerialListener() {
    std::string input = "";

    // use std::cin to read the input
    while (true) {
        std::cout << "LemLib > ";
        std::getline(std::cin, input);
        std::cout << std::endl;

        std::string command = input.substr(0, input.find(" "));
        std::vector<std::string> args;
        if (input.find(" ") != std::string::npos) {
            std::string argString = input.substr(input.find(" ") + 1);
            while (argString.find(" ") != std::string::npos) {
                args.push_back(argString.substr(0, argString.find(" ")));
                argString = argString.substr(argString.find(" ") + 1);
            }
            args.push_back(argString);
        }

        if (command == "index") {
            // read the index file
            std::vector<lemlibFile> index = readFileIndex();

            std::cout << "Index file" << std::endl;
            std::cout << "----------" << std::endl;
            std::cout << "Name | Sector" << std::endl;

            for (const lemlibFile& line : index) { std::cout << line.name << " | " << line.sector << std::endl; }
        } else if (command == "sector") {
            if (args.size() == 0) {
                std::cout << "Usage: sector <path>" << std::endl;
                continue;
            }

            std::string name = args[0].c_str();

            std::cout << "Location of sector " + name + ": " << getFileSector(name.c_str()) << std::endl;
        } else if (command == "ls") {
            if (args.size() == 0) {
                std::cout << "Usage: ls <path> [recursive]" << std::endl;
                continue;
            }

            std::string path = args[0].c_str();
            bool recursive = false;

            if (args.size() > 1)
                if (args[1] == "true") recursive = true;

            std::vector<std::string> files = listDirectory(path.c_str(), recursive);

            std::cout << "Files in " + path + ":" << std::endl;
            std::cout << "-----------------------" << std::endl;
            std::cout << "Name | Type" << std::endl;

            for (const std::string& file : files) {
                std::cout << file << " | " << (isDirectory(file.c_str()) ? "Directory" : "File") << std::endl;
            }

            std::cout << std::endl;
        } else if (command == "exists") {
            if (args.size() == 0) {
                std::cout << "Usage: exists <path>" << std::endl;
                continue;
            }

            std::string path = args[0].c_str();

            bool exists = fileExists(path.c_str());

            std::cout << "Exists: " + std::string(exists ? "true" : "false") << std::endl;
        } else if (command == "delete") {
            if (args.size() == 0) {
                std::cout << "Usage: delete <path>" << std::endl;
                continue;
            }

            std::string path = args[0].c_str();

            deleteFile(path.c_str());

            std::cout << "Deleted file " + path << std::endl;
        } else if (command == "create") {
            if (args.size() == 0) {
                std::cout << "Usage: create <path> [override]" << std::endl;
                continue;
            }

            std::string path = args[0].c_str();

            bool override = false;

            if (args.size() > 1)
                if (args[1] == "true") override = true;

            createFile(path.c_str(), override);

            std::cout << "Created file " + path << std::endl;
        } else if (command == "write") {
            if (args.size() == 0) {
                std::cout << "Usage: write <path> <data>" << std::endl;
                continue;
            }

            std::string path = args[0].c_str();

            std::string data = "";

            for (int i = 1; i < args.size(); i++) { data += args[i] + " "; }

            data = data.substr(0, data.length() - 1);

            write(path.c_str(), data.c_str());

            std::cout << "Wrote to file " + path << std::endl;
        } else if (command == "read") {
            if (args.size() == 0) {
                std::cout << "Usage: read <path>" << std::endl;
                continue;
            }

            std::string path = args[0].c_str();

            std::string data = read(path.c_str());

            std::cout << "Data in file " + path + ":" << std::endl;
            std::cout << "-----------------------" << std::endl;
            std::cout << data << std::endl;
        } else if (command == "help") {
            std::cout << "Available commands:" << std::endl;
            std::cout << "-----------------------" << std::endl;
            std::cout << "index" << std::endl;
            std::cout << "sector <path>" << std::endl;
            std::cout << "ls <path> [recursive]" << std::endl;
            std::cout << "exists <path>" << std::endl;
            std::cout << "delete <path>" << std::endl;
            std::cout << "create <path> [override]" << std::endl;
            std::cout << "write <path> <data>" << std::endl;
            std::cout << "read <path>" << std::endl;
            std::cout << "help" << std::endl;
            std::cout << "exit" << std::endl;
        } else if (command == "exit") {
            std::cout << std::endl;
            std::cout << "Exiting..." << std::endl;
            break;
        } else {
            std::cout << "Unknown command" << std::endl;
        }

        std::cout << std::endl;
    }
}

/**
 * @brief Main function
 *
 * @return int program exit code
 */
int main() {
    // Initialize the file system
    initVFS();
    std::cout << "[INIT] Initialized" << std::endl;

    initializeSerialListener();
}
