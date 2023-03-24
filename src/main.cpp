/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Module:       main.cpp                                                  */
/*    Author:       LemLib Team                                               */
/*    Created:      3/23/2023, 12:18:22 PM                                    */
/*    Description:  LemLib file system serial interpreter                     */
/*                                                                            */
/*----------------------------------------------------------------------------*/
#include "vex.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

// Exception codes:
// [E1] Micro SD card not inserted
// [E2] Could not open index file
// [E3] Could not open file
// [E4] Path must start with a slash

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

/**
 * @brief Initialize the file system
 *
 */
void initVFS() {
    // Check if the micro sd card is inserted
    vex::brain Brain;
    if (!Brain.SDcard.isInserted()) throw "[E1] Micro SD card not inserted";
    // Check if the index file exists
    std::ifstream indexFile;
    indexFile.open("index.txt");
    if (!indexFile.is_open()) throw "[E2] Could not open index file";
}

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
 * @brief Read the index file
 *
 * @return std::vector<lemlibFile> contents of the index file
 */
std::vector<lemlibFile> readFileIndex() {
    // throw an exception if the micro sd card is not inserted
    if (!vex::brain().SDcard.isInserted()) throw "[E1] Micro SD card not inserted";
    // Initialize the vector
    std::vector<lemlibFile> index;
    // Open the index file
    std::ifstream indexFile;
    indexFile.open("index.txt");
    // throw an exception if the index file could not be opened
    if (!indexFile.is_open()) throw "[E2] Could not open index file";
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
    // throw an exception if the path does not start with a slash
    if (path[0] != '/') throw "[E4] Path must start with a slash";
    // Read the index file
    std::vector<lemlibFile> index = readFileIndex();
    // Iterate through the index
    for (const lemlibFile& file : index) {
        // Check if the name matches
        if (file.name == path) return file.sector.c_str();
    }
    // Return null if the file is not found
    // also output an error for the LemLib extension
    std::cout << "[ERROR] File " << path << " not found" << std::endl;
    return NULL;
}

/**
 * @brief List all the files and folders in a directory
 *
 * @param dir the directory to list
 * @return std::vector <std::string> a vector of all the files and folders in the directory
 */
std::vector<std::string> listDirectory(const char* dir, bool recursive = false) {
    // throw an exception if the path does not start with a slash
    if (dir[0] != '/') throw "[E4] Path must start with a slash";
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
    // throw an exception if the path does not start with a slash
    if (path[0] != '/') throw "[E4] Path must start with a slash";
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
    // throw an exception if the path does not start with a slash
    if (path[0] != '/') throw "[E4] Path must start with a slash";
    // check if the file exists
    if (!fileExists(path)) {
        throw "[E5] File does not exist";
        return;
    }
    // empty the sector the file is stored in
    std::ofstream sector;
    sector.open(getFileSector(path));
    sector << "";
    sector.close();
    // remove the file from the index file
    std::vector<lemlibFile> index = readFileIndex();
    std::ofstream indexFile;
    indexFile.open("index.txt");
    if (!indexFile.is_open()) throw "[E2] Could not open index file";
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
    // throw an exception if the path does not start with a slash
    if (path[0] != '/') throw "[E4] Path must start with a slash";
    // Create the file in the index file
    std::ofstream indexFile;
    indexFile.open("index.txt", std::ios_base::app);
    if (!indexFile.is_open()) throw "[E2] Could not open index file";
    // Check if the file already exists
    if (fileExists(path)) {
        // If the file should be overwritten, delete the file
        if (overwrite) deleteFile(path);
        // Otherwise, throw an exception
        else throw "[E3] File " + std::string(path) + " already exists";
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
    return to_string(sector).c_str();
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
}
