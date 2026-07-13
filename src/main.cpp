#include "./business/MockBusinessClient.h"
#include "./config/ServerConfig.h"
#include "./reactor/MainReactor.h"
#include "./reactor/SubReactor.h"
#include "./threadpool/Threadpool.h"

#include <iostream>
#include <memory>
#include <vector>

int main() {
    ServerConfig config = ServerConfigLoader::load("config/server.conf");
    std::cout << "environment: " << config.environment << std::endl;
    std::cout << "port: " << config.port << std::endl;
    std::cout << "storage root: " << config.storage_root << std::endl;

    ThreadPool pool(config.thread_num);
    std::cout << "thread pool start, workers=" << config.thread_num << std::endl;

    MockBusinessClient business(config.storage_root);

    std::vector<std::unique_ptr<SubReactor>> owned_sub_reactors;
    std::vector<SubReactor*> sub_reactors;
    for (int i = 0; i < config.sub_reactor_num; ++i) {
        owned_sub_reactors.push_back(std::make_unique<SubReactor>(&pool, &business));
        sub_reactors.push_back(owned_sub_reactors.back().get());
    }
    std::cout << "sub reactors init, count=" << config.sub_reactor_num << std::endl;

    for (SubReactor* sub : sub_reactors) {
        sub->start();
    }
    std::cout << "sub reactors start" << std::endl;

    MainReactor main_reactor(sub_reactors, config.port);
    main_reactor.start();

    return 0;
}
