#include "dop.h"

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
        isLogged = true;
        clear_data(data);
        if(isLogged && isPrinted) reset_bool();
        else condition.notify_one();
    }
}

void printer()
{
    std::unique_lock<std::mutex> lck{conditionMutex};
    while(!finished)
    {
        while(data_for_print.empty() && !finished)
        {
            condition.wait(lck);
        }
        print_in_file(data_for_print, file_name, count_com);
        isPrinted = true;
        clear_data(data_for_print);
        if(isLogged && isPrinted) reset_bool();
        else condition.notify_one();
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
    std::thread log{logger};
    std::thread main{producer};
    std::thread file1{printer};
    main.join();
    log.join();
    file1.join();
}

int main()
{
    test_condition();
    return 0;
}
