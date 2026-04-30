#include <nlohmann/json_fwd.hpp>
#include <toml++/toml.hpp>

#include "asio/io_context.hpp"
#include "request.hpp"
#include "request_interceptor.hpp"
#include "server.hpp"

#include <asio.hpp>

#include <print>

class Controller {
	int test{0};

	public:
	Controller() = default;
};

struct TmpStruct{
	int val1 = 12;
	int val2 = 1515;
	std::string yooo = "asdfg";
};
void to_json(nlohmann::json &json, const TmpStruct &obj){
	json["val1"] = obj.val1;
	json["val2"] = obj.val2;
	json["yooo"] = obj.yooo;
}

int main() {
	asio::io_context io_context;
	Server serv{io_context, "127.0.0.1", 8110};
	serv.controllers.AddController<Controller>();
	serv.AddFilter([](HttpRequest &request){
		if(request.header.path.starts_with("/api")){
			return false;
		}

		return true;
	});
	serv.controllers.AddHandler("/user/register", std::function<TmpStruct()>([](){
		std::println("Registration attempted");
		return TmpStruct{};
	}));

	serv.Start();
	while(true){
		io_context.run();
	}

	return 0;
}