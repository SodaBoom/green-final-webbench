#include <iostream>
#include <cstring>
#include <algorithm>
#include <unordered_map>

#include "cinatra/include/cinatra.hpp"

using namespace std;

int main() {
	int max_thread_num = std::thread::hardware_concurrency();
	cinatra::http_server server(max_thread_num);
	server.listen("0.0.0.0", "8080");
	server.set_http_handler<cinatra::GET, cinatra::POST>("/collect_energy/*", [](cinatra::request& req, cinatra::response& res) {
		res.set_status_and_content(cinatra::status_type::ok, "true");
	});

	server.run();
	return 0;
}
