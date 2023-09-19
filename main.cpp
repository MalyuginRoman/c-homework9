#include <iostream>
#include <windows.h>
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
    std::mutex conditionMutex;
    std::condition_variable condition;

    command_block static_block;
    command_block dynamic_block;
    std::vector<std::string> data;
    std::string file_name;
    size_t count_com;
    std::string text;

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
        file_name = vector.file_name;
        count_com = vector.count;
    }

    std::string GetFileName()
    {
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
        file_time.time_since_epoch()).count();
        std::stringstream filename;
        filename << "bulk" << seconds << ".log"; // log in thread
        return filename.str();
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

    void logger()
    {
        std::unique_lock<std::mutex> lck{conditionMutex};
        while(!finished)
        {
            while(data.empty() && !finished)
            {
                condition.wait(lck);
            }
            print_in_log(data);
            print_in_file(data, file_name, count_com);
            data.clear();
            finished = false;
        }
    }

    void producer()
    {
        const int max_size = 3;
        while(!finished)
        {
            {
                std::lock_guard<std::mutex> guard{conditionMutex};
                {
                    file_time = std::chrono::system_clock::now();
                    std::cin >> text;
                    if(!isActDyn)                                   // static block
                    {
                        if(text != "EOF" && text != "{")            // text != "EOF" && text != "{"
                        {
                            if (!static_block.count)
                                static_block.file_name = GetFileName();
                            static_block.command.emplace_back(text);
                            static_block.count = static_block.command.size();
                            if(static_block.count == max_size)
                            {
                                replace_data(static_block);
                                Synhronize(static_block);
                                condition.notify_one();
                                finished = true;
                            }
                        }
                        if(text == "EOF" || text == "{")            // text == "EOF" || text == "{"
                        {
                            static_block.count = static_block.command.size();
                            replace_data(static_block);
                            Synhronize(static_block);
                            if(static_block.count)
                            {
                                condition.notify_one();
                                finished = true;
                            }
                            if(text == "{")
                            {
                                isActDyn = true;
                                dynamicCount ++;
                            }
                        }
                    }
                    else                                            // dynamic block
                    {
                        if(text != "{" && text != "}" && text != "EOF")
                        {
                            if (!dynamic_block.count)
                                dynamic_block.file_name = GetFileName();
                            dynamic_block.command.emplace_back(text);
                            dynamic_block.count = dynamic_block.command.size();
                        }
                        if(text == "{") dynamicCount ++;
                        if(text == "}") dynamicCount --;
                        if(text == "EOF")
                            dynamic_block.command.clear();
                        if (dynamicCount == 0)                      // end of dynamic block
                        {
                            replace_data(dynamic_block);
                            Synhronize(dynamic_block);
                            condition.notify_one();
                            isActDyn = false;
                            finished = true;
                        }
                    }
                }
            }
            condition.notify_one();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        condition.notify_one();
    }

    void test_condition()
    {
        std::cout << "Start programm" << std::endl;
        std::thread loggerThread{logger};
        std::thread produserThread{producer};
        produserThread.join();
        loggerThread.join();
    }

int main()
{
    test_condition();
    return 0;
}
