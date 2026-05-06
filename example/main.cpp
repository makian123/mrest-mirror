#include <annotations/controller.hpp>
#include <annotations/request.hpp>
#include <asio.hpp>
#include <glaze/json/write.hpp>
#include <nlohmann/json_fwd.hpp>
#include <print>
#include <toml++/toml.hpp>

#include "annotations/parameters.hpp"
#include "asio/awaitable.hpp"
#include "asio/io_context.hpp"
#include "cookie.hpp"
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
using namespace mrest::annotation;
using namespace mrest::exception;
class[[=RestController("/public")]] Controller {
	std::unordered_map<std::string, std::string> users;

   public:
	Controller() = default;

	[[=GetRequest("/info")]]
	asio::awaitable<std::string> UserInfo([[=RequestParam("username")]] std::string username,
	[[=RequestCookies]] const mrest::CookieContainer &cookies) {
		if(auto cookie = cookies.GetCookie("token"); cookie){
			std::println("Token: {}", (*cookie)->value);
		}
		else{
			throw BadRequestException("No token cookie");
		}
		
		if(!users.contains(username)){
			throw BadRequestException("User not found");
		}

		co_return users[username];
	}
	[[=GetRequest("/info/{username}")]]
	asio::awaitable<std::string> PathUserInfo([[=RequestBody]] const AuthRequestDto &body, 
		[[=PathVariable("username")]] const std::string &username, 
		[[=RequestParam("token")]] const std::string &token) {
		if(!users.contains(username)){
			throw BadRequestException("User not found");
		}
		if(token.empty()){
			throw BadRequestException("Not authorized");
		}

		std::string bodyJson;
		auto ignored = glz::write_json(body, bodyJson);
		std::println("Body: {}", bodyJson);
		std::println("Token: {}", token);
		co_return users[username];
	}

	[[=PostRequest("/login")]]
	asio::awaitable<TmpStruct> TestRequest([[=RequestBody]] const AuthRequestDto &request, [[=RequestSession]] const mrest::HttpSession &session) {
		std::println("Session id: {}", session.GetId());
		auto userIt = std::find_if(users.begin(), users.end(),
								   [&request](const std::pair<std::string, std::string> &usrInfo) {
									   return usrInfo.first == request.username &&
											  usrInfo.second == request.password;
								   });

		if (userIt == users.end()) {
			throw BadRequestException("Bad credentials");
		}

		co_return TmpStruct{};
	}
	[[=PostRequest("/register")]]
	asio::awaitable<void> RegisterHandler([[=RequestBody]] const AuthRequestDto &request) {
		if (users.contains(request.username)) {
			throw BadRequestException("User exists");
		}

		users.emplace(request.username, request.password);
		co_return;
	}
};

int main() {
	asio::io_context io_context;
	mrest::Server serv{io_context, "configuration.toml"};
	serv.AddFilter([](mrest::HttpRequest &request) {
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