#include <request.hpp>
#include <sstream>

namespace {
    std::string_view trim(std::string_view sv) {
        size_t start = 0;
        size_t end = sv.size();
    
        while (start < end && isspace(sv[start])) ++start;
        while (end > start && isspace(sv[end - 1])) --end;
    
        return sv.substr(start, end - start);
    }
}

HttpHeader::HttpHeader(const std::string &rawRequest){
    std::istringstream stream{rawRequest};
    std::string line;

    stream >> method >> path >> version;

    // Deal with headers
    while(std::getline(stream, line)){
        if(!line.empty() && line.back() == '\r'){
            line.pop_back();
        }

        if(line.empty()){
            break;
        }

        auto delimPos = line.find(':');
        if(delimPos == std::string::npos){
            continue;
        }

        auto key = trim(std::string_view(line.c_str(), delimPos));
        auto value = trim(std::string_view(line.c_str() + delimPos + 1, line.size() - delimPos - 1));

        headers[std::string{key}] = std::string{value};
    }

    // Parameter trimming
    auto paramStartIt = path.find('?');
    if(paramStartIt == std::string::npos){
        return;
    }

    auto paramString = path.substr(paramStartIt);
    std::istringstream paramStream{paramString};
    std::string pair;
    while(std::getline(paramStream, pair, '&')){
        auto eqIt = pair.find('=');
        if(eqIt == std::string::npos){
            continue;
        }

        auto key = trim(std::string_view(pair.c_str(), eqIt));
        auto value = trim(std::string_view(pair.c_str() + eqIt + 1, pair.size() - eqIt - 1));
        parameters[std::string{key}] = std::string{value};
    }
    path.erase(paramStartIt);
}
HttpRequest::HttpRequest(const std::string &rawRequest): header{rawRequest}{
    auto bodyIt = rawRequest.find("\r\n\r\n");
    if(bodyIt == std::string::npos){
        return;
    }

    body = std::string{rawRequest.begin() + bodyIt + 2, rawRequest.end()};
}
