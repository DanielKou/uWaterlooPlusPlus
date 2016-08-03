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

std::string nullCheck(json j) {
	if (j.is_null())
		return "";
	else
		return j.get<std::string>();
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
	EventsParser ep(result, apiKey);
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
Parser::Parser(std::string json, std::string apiKey) : raw(json), apiKey(apiKey) {
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
EventsParser::EventsParser(std::string eventsJson, std::string apiKey) : Parser(eventsJson, apiKey) {
	parser = parser["data"];
	length = parser.size();

	for (json::iterator it = parser.begin(); it != parser.end(); ++it) {
		int id = (*it)["id"];
		std::string site = (*it)["site"];

		Event *e = new Event((*it).dump(), apiKey);
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


std::vector<std::pair<std::string, std::string> > EventsParser::getHolidays() {
	std::string tempResult;
	std::vector<std::pair<std::string, std::string> > v;

	CURL* tempCurl = curl_easy_init();
	if (tempCurl) {
		curl_easy_setopt(tempCurl, CURLOPT_WRITEFUNCTION, writeCallback);
		curl_easy_setopt(tempCurl, CURLOPT_WRITEDATA, &tempResult);
		curl_easy_setopt(tempCurl, CURLOPT_URL, ("https://api.uwaterloo.ca/v2/events/holidays.json?key=" + apiKey).c_str());
		curl_easy_perform(tempCurl);
		json tempParser;
		std::stringstream ss;
		ss << tempResult;
		ss >> tempParser;
		tempParser = tempParser["data"];

		for (json::iterator it = tempParser.begin(); it != tempParser.end(); ++it) {
			v.push_back(std::make_pair((*it)["name"].get<std::string>(), (*it)["date"].get<std::string>()));
		}
		curl_easy_cleanup(tempCurl);
	}
	return v;
}


//------------------------------ EVENT ------------------------------------------------
//-------------------------------------------------------------------------------------
Event::Event(std::string eventJson, std::string apiKey, bool specific /* = false */) : Parser(eventJson, apiKey){
	id = parser["id"];
	title = parser["title"].get<std::string>(); //explicit string conversions
	site = nullCheck((specific) ? parser["site_id"] : parser["site"]);
	siteName = nullCheck(parser["site_name"]);
	link = parser["link"].get<std::string>();
	updatedTime = parser["updated"].get<std::string>();
}


SpecificEvent Event::getSpecifics() {
	std::string tempResult;
	CURL* tempCurl = curl_easy_init();
	if (tempCurl) {
		curl_easy_setopt(tempCurl, CURLOPT_WRITEFUNCTION, writeCallback);
		curl_easy_setopt(tempCurl, CURLOPT_WRITEDATA, &tempResult);
		curl_easy_setopt(tempCurl, CURLOPT_URL, ("https://api.uwaterloo.ca/v2/events/" 
												+ site + std::string("/") + std::to_string(id) 
												+ std::string(".json?key=") + apiKey).c_str());
		curl_easy_perform(tempCurl);
		curl_easy_cleanup(tempCurl);

		json tempJson;
		std::stringstream ss;
		ss << tempResult;
		ss >> tempJson;
		SpecificEvent se(tempJson["data"].dump(), apiKey);
		return se;
	}
}


// Getters
int Event::getId() { return id; }
std::string Event::getTitle() { return title; }
std::string Event::getSite() { return site; }
std::string Event::getSiteName() { return siteName; }
std::string Event::getLink() { return link; }
std::string Event::getUpdatedTime() { return updatedTime; }



//------------------------------ SPECIFIC EVENT ---------------------------------------
//-------------------------------------------------------------------------------------
SpecificEvent::SpecificEvent(std::string eventJson, std::string apiKey) : Event(eventJson, apiKey, true) {
	description = parser["description"].get<std::string>();
	descriptionRaw = parser["description_raw"].get<std::string>();
	cost = (parser["cost"].is_null()) ? 0 : parser["cost"];
	json temp = parser["times"];
	for (json::iterator it = temp.begin(); it != temp.end(); ++it) {
		times.push_back(std::make_pair((*it)["start"].get<std::string>(), (*it)["end"].get<std::string>()));
	}

	temp = parser["type"];
	int size = temp.size();
	for (int i = 0; i < size; i++) {
		type += temp[i].get<std::string>();
		if (i != 0) {
			type += ", ";
		}
	}

	temp = parser["location"];
	location.name = temp["name"].get<std::string>();
	location.street = temp["street"].get<std::string>();
	location.city = temp["city"].get<std::string>();
	location.province = temp["province"].get<std::string>();
	location.postalCode = temp["postal_code"].get<std::string>();
}


// getters
std::string SpecificEvent::getDescription() { return description; }
std::string SpecificEvent::getDescriptionRaw() { return descriptionRaw; }
double SpecificEvent::getCost() { return cost; }
std::vector<std::pair<std::string, std::string> > SpecificEvent::getTimes() { return times; }
std::string SpecificEvent::getType() { return type; }
Location SpecificEvent::getLocation() { return location; }