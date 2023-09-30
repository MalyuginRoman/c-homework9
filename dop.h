#include <iostream>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>

struct command_block
{
    std::vector<std::string> command;
    size_t count = 0;
    std::string file_name;
};

std::atomic<bool> finished{false};
std::mutex conditionMutex_Log;
std::condition_variable condition_Log;
std::mutex conditionMutex_Print_s;
std::condition_variable condition_Print_s;
std::mutex conditionMutex_Print_d;
std::condition_variable condition_Print_d;

command_block static_block;
command_block dynamic_block;
std::vector<std::string> data;
std::vector<std::string> data_for_print;
std::string file_name;
size_t count_com;
std::string text;

bool isLogged = false;                  //
bool isPrinted = false;                 //
bool isActDyn = false;
std::chrono::system_clock::time_point file_time;
int dynamicCount = 0;

void Synhronize(command_block &vector)
{
    vector.command.clear();
    vector.count = vector.command.size();
}

void print_in_log(std::vector<std::string> data)
{
    std::cout << "bulk: ";
    size_t i = 0;
    size_t count = data.size();
    for (auto v : data)
    {
        i++;
        std::cout << v;
        if(i != count) std::cout << ", ";
    }
    std::cout << std::endl;
}

void replace_data(command_block &vector)
{
    data = vector.command;
    data_for_print = data;
    file_name = vector.file_name;
    count_com = vector.count;
}

std::string GetFileName()
{
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
    file_time.time_since_epoch()).count();
    std::stringstream filename;
    std::string metka;
    filename << "bulk" << seconds; // << ".log"; // log in thread
    return filename.str();
}

std::string GetThreadID()
{
    std::stringstream ss;
    ss << std::this_thread::get_id();
    int thread_id = std::stoi(ss.str());
    std::string new_name = std::to_string(thread_id);
    return new_name;
}

void print_in_file(std::vector<std::string> data, std::string name, size_t count)
{
    std::ofstream out;
    out.open(name);
    if(out.is_open()) {
        for(size_t i = 0; i < count; ++i)
        {
            out << data[i];
            if (i < count - 1) out << ", ";
        }
        out.close();
    }
}

void clear_data(std::vector<std::string> &data)
{
    data.clear();
    finished = false;
}
