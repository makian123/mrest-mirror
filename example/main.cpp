#include <nlohmann/json_fwd.hpp>
#include <toml++/toml.hpp>

#include "asio/io_context.hpp"
#include "request.hpp"
#include "request_interceptor.hpp"
#include "server.hpp"

#include <annotations/request.hpp>

#include <asio.hpp>

#include <print>

class Controller {
	int test{0};

	public:
	Controller() = default;

	[[=GetRequest("/api")]]
	void TestRequest(){
		std::println("Api called");
	}
};

struct TmpStruct{
	int val1 = 12;
	int val2 = 1515;
	std::string yooo = "asdfg";
};

int main() {
	asio::io_context io_context;
	Server serv{io_context, "127.0.0.1", 8110};
	serv.AddFilter([](HttpRequest &request){
		if(request.header.path.starts_with("/api")){
			return false;
		}
		
		return true;
	});
	
	serv.controllers.AddController<Controller>();
	
	serv.Start();
	while(true){
		io_context.run();
	}

	return 0;
}