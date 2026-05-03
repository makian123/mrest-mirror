#include <nlohmann/json_fwd.hpp>
#include <toml++/toml.hpp>

#include "annotations/parameters.hpp"
#include "asio/io_context.hpp"
#include "request.hpp"
#include "request_interceptor.hpp"
#include "server.hpp"

#include <annotations/request.hpp>
#include <annotations/controller.hpp>

#include <asio.hpp>

#include <print>

struct TmpStruct{
	int val1 = 123;
	int val2 = 1515;
	std::string val3 = "asd";
};

struct LoginRequestDto {
	std::string username;
	std::string password;
};
class [[=RestController("/public")]] Controller {
	int test{0};

	public:
	Controller() = default;

	[[=PostRequest("/login")]]
	TmpStruct TestRequest([[=RequestBody{}]] const LoginRequestDto &request){
		std::println("Login called");
		std::println("Username: {}", request.username);
		std::println("Password: {}", request.password);

		return TmpStruct{};
	}
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