#include "uWaterloo++.h"


static size_t writeCallback(void *Contents, size_t size, size_t nmemb, void *userp) {
	((std::string *)userp)->append((char*)Contents);
	return size * nmemb;
}


// generic map<typeA, typeB> ==> vector<typeB> conversion
template <typename M, typename V>
void MapToVec(const  M & m, V & v) {
	for (typename M::const_iterator it = m.begin(); it != m.end(); ++it) {
		v.push_back(it->second);
	}
}


//------------------------------ UWaterloo API Wrapper --------------------------------
//-------------------------------------------------------------------------------------
uWaterloo::uWaterloo(std::string key) : apiKey(key) {}


bool uWaterloo::init() {
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
		return true;
	}
	return false;
}


EventsParser uWaterloo::parseEvents() {
	std::string url = "https://api.uwaterloo.ca/v2/events.json?key=" + apiKey;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	res = curl_easy_perform(curl);
	
	// temp fix: clean up response, sometimes it screws up and appends garbage value (maybe look into why it happens)
	size_t found = result.find_last_of("}");
	result = result.substr(0, found + 1);
	EventsParser ep(result);
	return ep;
}

bool uWaterloo::cleanUp() {
	if (curl && res) {
		curl_easy_cleanup(curl);
		return true;
	}
	return false;
}



//------------------------------ PARSER SUPERCLASS ------------------------------------
//-------------------------------------------------------------------------------------
Parser::Parser(std::string json) : raw(json) {
	// initialize json object
	std::stringstream ss;
	ss << json;
	ss >> parser;
}


std::string Parser::getRaw() {
	return raw;
}


std::ostream& operator << (std::ostream &out, Parser &p) {
	out << p.raw;
	return out;
}



//------------------------------ EVENTS PARSER ----------------------------------------
//-------------------------------------------------------------------------------------
EventsParser::EventsParser(std::string eventsJson) : Parser(eventsJson) {
	parser = parser["data"];
	length = parser.size();

	for (json::iterator it = parser.begin(); it != parser.end(); ++it) {
		int id = (*it)["id"];
		std::string site = (*it)["site"];

		Event *e = new Event((*it).dump());
		events[site].insert(std::make_pair(id, e));
	}
}


int EventsParser::getLength() {
	return length;
}






std::vector<Event*> EventsParser::findEventsBySite(std::string site) {
	std::vector<Event*> v;
	
	if (events.find(site) != events.end()) {
		MapToVec(events[site], v);
	}
	return v;
}


std::vector<Event*> EventsParser::findEventsById(int id) {
	std::vector<Event*> v;

	for (std::map<std::string, std::map<int, Event*> >::iterator it = events.begin(); it != events.end(); it++) {
		std::map<int, Event*> temp = it->second;
		for (std::map<int, Event*>::iterator it2 = temp.begin(); it2 != temp.end(); it2++) {
			if (it2->first == id) {
				v.push_back(it2->second);
			}
		}
	}
	return v;
}


Event* EventsParser::findEvent(int id, std::string site) {
	std::map<std::string, std::map<int, Event*> >::iterator it = events.find(site);
	if (it != events.end() && it->second.find(id) != it->second.end()) {
		return (it->second.find(id))->second;
	}
	else {
		return NULL;
	}
}


std::vector<Event> EventsParser::getEventsList() {
	std::vector<Event> v;

	for (std::map<std::string, std::map<int, Event*> >::iterator it = events.begin(); it != events.end(); it++) {
		std::map<int, Event*> temp = it->second;
		for (std::map<int, Event*>::iterator it2 = temp.begin(); it2 != temp.end(); it2++) {
			Event e = *(it2->second);
			v.push_back(e);
		}
	}
	return v;
}



//------------------------------ EVENT ------------------------------------------------
//-------------------------------------------------------------------------------------
Event::Event(std::string eventJson) : Parser(eventJson){
	id = parser["id"];
	title = parser["title"].get<std::string>(); //explicit string conversions
	site = parser["site"].get<std::string>();
	siteName = parser["site_name"].get<std::string>();
	link = parser["link"].get<std::string>();
}


// Getters
std::string Event::getTitle() { return title; }
std::string Event::getSite() { return site; }
std::string Event::getSiteName() { return siteName; }
std::string Event::getLink() { return link; }
std::string Event::getUpdatedTime() { return updatedTime; }