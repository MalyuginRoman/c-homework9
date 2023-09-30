#include "dop.h"

void logger()
{
    std::unique_lock<std::mutex> lck{conditionMutex_Log};
    while(!finished)
    {
        while(data.empty() && !finished)
        {
            condition_Log.wait(lck);
        }
        print_in_log(data);
        isLogged = true;
        clear_data(data);
    }
}

void printer_static()
{
    std::unique_lock<std::mutex> lck{conditionMutex_Print_s};
    while(!finished)
    {
        while(data_for_print.empty() && !finished)
        {
            condition_Print_s.wait(lck);
        }
        std::string thread_id = GetThreadID();
        std::string new_name = file_name + "_" + thread_id + ".log";
        print_in_file(data_for_print, new_name, count_com);
        isPrinted = true;
        clear_data(data_for_print);
    }
}

void printer_dynamic()
{
    std::unique_lock<std::mutex> lck{conditionMutex_Print_d};
    while(!finished)
    {
        while(data_for_print.empty() && !finished)
        {
            condition_Print_d.wait(lck);
        }
        std::string thread_id = GetThreadID();
        std::string new_name = file_name + "_" + thread_id + ".log";
        print_in_file(data_for_print, new_name, count_com);
        isPrinted = true;
        clear_data(data_for_print);
    }
}

void producer()
{
    const int max_size = 3;
    while(!finished)
    {
        {
            std::lock_guard<std::mutex> guard1{conditionMutex_Log};
            std::lock_guard<std::mutex> guard2{conditionMutex_Print_s};
            std::lock_guard<std::mutex> guard3{conditionMutex_Print_d};
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
                            condition_Log.notify_one();
                            condition_Print_s.notify_one();
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
                            condition_Log.notify_one();
                            condition_Print_s.notify_one();
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
                        condition_Log.notify_one();
                        condition_Print_d.notify_one();
                        isActDyn = false;
                        finished = true;
                    }
                }
            }
        }
        condition_Log.notify_one();
        condition_Print_s.notify_one();
    }
    condition_Log.notify_one();
    condition_Print_s.notify_one();
}

void test_condition()
{
    std::cout << "Start programm" << std::endl;
    std::thread log{logger};
    std::thread main{producer};
    std::thread file1{printer_static};
    std::thread file2{printer_dynamic};
    main.join();
    log.join();
    file1.join();
    file2.join();
}

int main()
{
    test_condition();
    return 0;
}
