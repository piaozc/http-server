#include "./business/MockBusinessClient.h"
#include "./reactor/MainReactor.h"
#include "./reactor/SubReactor.h"
#include "./threadpool/Threadpool.h"

#include <iostream>
#include <memory>
#include <vector>

int main() {
    int thread_num = 12;
    ThreadPool pool(thread_num);
    std::cout << "thread pool start" << std::endl;

    MockBusinessClient business("./storage");

    int sub_reactor_num = 3;
    std::vector<std::unique_ptr<SubReactor>> owned_sub_reactors;
    std::vector<SubReactor*> sub_reactors;
    for (int i = 0; i < sub_reactor_num; ++i) {
        owned_sub_reactors.push_back(std::make_unique<SubReactor>(&pool, &business));
        sub_reactors.push_back(owned_sub_reactors.back().get());
    }
    std::cout << "sub reactors init" << std::endl;

    for (SubReactor* sub : sub_reactors) {
        sub->start();
    }
    std::cout << "sub reactors start" << std::endl;

    int port = 10889;
    MainReactor main_reactor(sub_reactors, port);
    main_reactor.start();

    return 0;
}
