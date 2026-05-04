#include <annotations/controller.hpp>
#include <annotations/request.hpp>
#include <asio.hpp>
#include <glaze/json/write.hpp>
#include <nlohmann/json_fwd.hpp>
#include <print>
#include <toml++/toml.hpp>

#include "annotations/parameters.hpp"
#include "asio/io_context.hpp"
#include "exceptions/bad_request.hpp"
#include "request.hpp"
#include "request_interceptor.hpp"
#include "server.hpp"

struct TmpStruct {
	int val1 = 123;
	int val2 = 1515;
	std::string val3 = "asd";
};

struct AuthRequestDto {
	std::string username;
	std::string password;
};
class[[=RestController("/public")]] Controller {
	std::unordered_map<std::string, std::string> users;

   public:
	Controller() = default;

	[[=GetRequest("/info")]]
	std::string UserInfo([[=RequestParam("username")]] std::string username) {
		if(!users.contains(username)){
			throw BadRequestException("User not found");
		}
		return users[username];
	}
	[[=GetRequest("/info/{username}")]]
	std::string PathUserInfo([[=RequestBody]] const AuthRequestDto &body, 
		[[=PathVariable("username")]] const std::string &username, 
		[[=RequestParam("token")]] const std::string &token) {
		if(!users.contains(username)){
			throw BadRequestException("User not found");
		}
		if(token.empty()){
			throw BadRequestException("Not authorized");
		}

		std::string bodyJson;
		glz::write_json(body, bodyJson);
		std::println("Body: {}", bodyJson);
		std::println("Token: {}", token);
		return users[username];
	}

	[[=PostRequest("/login")]]
	TmpStruct TestRequest([[=RequestBody]] const AuthRequestDto &request) {
		auto userIt = std::find_if(users.begin(), users.end(),
								   [&request](const std::pair<std::string, std::string> &usrInfo) {
									   return usrInfo.first == request.username &&
											  usrInfo.second == request.password;
								   });

		if (userIt == users.end()) {
			throw BadRequestException("Bad credentials");
		}

		return TmpStruct{};
	}
	[[=PostRequest("/register")]]
	void RegisterHandler([[=RequestBody]] const AuthRequestDto &request) {
		if (users.contains(request.username)) {
			throw BadRequestException("User exists");
		}

		users.emplace(request.username, request.password);
	}
};

int main() {
	asio::io_context io_context;
	Server serv{io_context, "127.0.0.1", 8110};
	serv.AddFilter([](HttpRequest &request) {
		if (request.header.path.starts_with("/api")) {
			return false;
		}

		return true;
	});

	serv.controllers.AddController<Controller>();

	serv.Start();
	io_context.run();

	return 0;
}