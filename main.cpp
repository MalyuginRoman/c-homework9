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
    std::string text;

    bool isActDyn = false;
    std::chrono::system_clock::time_point file_time;
    int dynamicCount = 0;

    void Synhronize(command_block &vector)
    {
        vector.command.clear();
        vector.count = vector.command.size();
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
            std::cout << "logger - start" << std::endl;
            if(!isActDyn) {
                size_t count = static_block.count;
                for (size_t i = 0; i < count; i++)
                {
                    data[i] = static_block.command[i];
                }
            }
            if(isActDyn) {
                size_t count = dynamic_block.count;
                for (size_t i = 0; i < count; i++)
                {
                    data[i] = dynamic_block.command[i];
                }
            }
            std::cout << "bulk: " << std::endl;
            for (auto v : data)
            {
                std::cout << v << ", ";
            }
            std::cout << std::endl;
            data.clear();
            finished = false;
        }
        std::cout << "logger - finished!" << std::endl;
    }

    void producer()
    {
        std::cout << "producer - start" << std::endl;
        const int max_size = 3;
        while(!finished)
        {
            {
                std::lock_guard<std::mutex> guard{conditionMutex};
                {
                    std::cin >> text;
                    if(!isActDyn)                                   // static block
                    {
                        if(text != "EOF" && text != "{")            // text != "EOF" && text != "{"
                        {
                            static_block.command.emplace_back(text);
                            static_block.count = static_block.command.size();
                            if(static_block.count == max_size)
                            {
                                data = static_block.command;
                                Synhronize(static_block);
                                condition.notify_one();
                                finished = true;
                                std::cout << "END STATIC BLOCK \n";
                            }
                        }
                        if(text == "EOF" || text == "{")            // text == "EOF" || text == "{"
                        {
                            static_block.count = static_block.command.size();
                            if(static_block.count)
                                condition.notify_one();
                            data = static_block.command;
                            Synhronize(static_block);
                            std::cout << "END STATIC BLOCK \n";
                            if(text == "{")
                            {
                                std::cout << "S?TART DYNAMIC BLOCK \n";
                                isActDyn = true;
                                dynamicCount ++;
                            }
                        }
                    }
                    else                                            // dynamic block
                    {
                        if(text != "{" && text != "}" && text != "EOF")
                        {
                            dynamic_block.command.push_back(text);
                            dynamic_block.count = dynamic_block.command.size();
                        }
                        if(text == "{") dynamicCount ++;
                        if(text == "}") dynamicCount --;
                        if(text == "EOF")
                            dynamic_block.command.clear();
                        if (dynamicCount == 0)                      // end of dynamic block
                        {
                            Synhronize(dynamic_block);
                            isActDyn = false;
                            condition.notify_one();
                            finished = true;
                            std::cout << "END DYNAMIC BLOCK \n";
                        }
                    }
                }
            }
            condition.notify_one();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        condition.notify_one();
        std::cout << "producer - finished!" << std::endl;
    }

    void test_condition()
    {
        std::cout << "Start programm" << std::endl;
        std::thread loggerThread{logger};
        std::thread produserThread{producer};
        std::this_thread::sleep_for(std::chrono::seconds(3));
//        finished = true;
        produserThread.join();
        loggerThread.join();
    }

int main()
{
    test_condition();
    return 0;
}
