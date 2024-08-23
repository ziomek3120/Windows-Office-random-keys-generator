#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <iomanip>
#include <sys/stat.h>
#include <thread>
#include <vector>
#include <mutex>
#include <sstream>
#include <iomanip>

bool createDirectory(const char* path) {
    #if defined(_WIN32)
        int result = mkdir(path);
    #else
        mode_t mode = 0755; // rwxr-xr-x
        int result = mkdir(path, mode);
    #endif
    return result == 0;
}

std::string generateKey() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 35); // Increase range to include numbers as well

    std::string key;
    for (int i = 0; i < 25; ++i) {
        int charType = dis(gen) < 26 ? 0 : 1; // 0 for letter, 1 for number
        if (charType == 0) {
            key += static_cast<char>('A' + (dis(gen) % 26)); // Add a random letter
        } else {
            key += std::to_string(dis(gen) % 10); // Add a random number
        }
    }

    // Insert dashes at appropriate positions
    key.insert(5, "-");
    key.insert(11, "-");
    key.insert(17, "-");
    key.insert(23, "-");

    return key;
}

void generateKeysToFile(const std::string& directoryName, int keysPerFile, int startFileNum, int endFileNum, std::mutex& mtx, int& filesGenerated) {
    for (int fileNum = startFileNum; fileNum < endFileNum; ++fileNum) {
        std::ostringstream oss;
        oss << directoryName << "/key" << std::setfill('0') << std::setw(6) << fileNum << ".txt";
        std::string fileName = oss.str();

        std::ofstream file(fileName);

        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << fileName << std::endl;
            return;
        }

        for (int i = 0; i < keysPerFile; ++i) {
            file << generateKey() << std::endl;
        }
        file.close();

        // Update progress
        mtx.lock();
        ++filesGenerated;
        double progress = static_cast<double>(filesGenerated) / 10000 * 100;
        std::cout << "\rProgress: [" << std::string(static_cast<int>(progress), '=') << std::string(100 - static_cast<int>(progress), ' ') << "] " << std::fixed << std::setprecision(2) << progress << "%";
        std::cout.flush();
        mtx.unlock();
    }
}

int main() {
    const int totalFiles = 10000;
    const int keysPerFile = 132;
    const int numThreads = std::thread::hardware_concurrency(); // Get the number of hardware threads

    const char* directoryName = "keys";
    if (!createDirectory(directoryName)) {
        std::cerr << "Failed to create directory: " << directoryName << std::endl;
        return 1;
    }

    std::vector<std::thread> threads;
    int filesPerThread = totalFiles / numThreads;
    std::mutex mtx;
    int filesGenerated = 0;

    for (int t = 0; t < numThreads; ++t) {
        int startFileNum = t * filesPerThread;
        int endFileNum = (t == numThreads - 1) ? totalFiles : (t + 1) * filesPerThread;
        threads.emplace_back(generateKeysToFile, directoryName, keysPerFile, startFileNum, endFileNum, std::ref(mtx), std::ref(filesGenerated));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "\nKeys generation completed!" << std::endl;

    return 0;
}
