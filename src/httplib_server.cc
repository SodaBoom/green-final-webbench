#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <cmath>
#include "httplib.h"

#include "mariadb/conncpp.hpp"

using namespace std;
using namespace httplib;

void collect(const char *user_id, int to_collect_energy_id) {
    // do nothing
}

int main() {
    // 初始化server
    Server svr;
    // POST /collect_energy/{userId}/{toCollectEnergyId}
    svr.Get(R"(/collect_energy/(\w+)/(\d+))", [](const Request &req, Response &res) {
        auto userId = req.matches[1].str().c_str();
        auto toCollectEnergyId = atoi(req.matches[2].str().c_str());
        collect(userId, toCollectEnergyId);
        res.set_content("true", "text/plain");
    });
    svr.Post(R"(/collect_energy/(\w+)/(\d+))", [](const Request &req, Response &res) {
        auto userId = req.matches[1].str().c_str();
        auto toCollectEnergyId = atoi(req.matches[2].str().c_str());
        collect(userId, toCollectEnergyId);
        res.set_content("true", "text/plain");
    });

    if (!svr.bind_to_port("0.0.0.0", 8080)) {
        std::cerr << "bind port fail" << std::endl;
    }
    if (!svr.listen_after_bind()) {
        std::cerr << "listen fail" << std::endl;
    }
    return 0;
}
