#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <curl/curl.h>
#include "json.hpp"


using json = nlohmann::json;
using namespace std;

static size_t writeCallback(void *Contents, size_t size, size_t nmemb, void *userp) {
	((std::string *)userp)->append((char*)Contents);
	return size * nmemb;
}


int main() {
	// CURL stuff
	CURL *curl = curl_easy_init();
	string result;
	std::string url = "https://api.uwaterloo.ca/v2/weather/current.json?key=82873b516c87479fc4f3dc82f5fb20f2";
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
	CURLcode res = curl_easy_perform(curl);

	json j;
	stringstream ss;
	ss << result;
	ss >> j;


	cout << j.dump(4);



	int asd = 0;

}