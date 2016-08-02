#ifndef UWATERLOOPLUSPLUS_H
#define UWATERLOOPLUSPLUS_H

#include <iostream>
#include <curl/curl.h>
#include <string>
#include "json.hpp"
#include <map>
#include <vector>
#include <iostream>
#include <sstream>
using json = nlohmann::json;

class uWaterloo;
class EventsParser;
class Event;


class uWaterloo {
	CURL *curl;
	CURLcode res;
	const std::string apiKey;
	std::string result;

public:
	uWaterloo(std::string key);
	bool init();
	bool cleanUp();
	EventsParser parseEvents();
};


class Parser {
	const std::string raw;
protected:
	json parser;

public:
	Parser(std::string json);
	std::string getRaw();
	friend std::ostream& operator<<(std::ostream& out, Parser &p);
};

// NOTE: are copy constructor + assignment operator needed? shallow copy is ok, since all info is read only,
// nothing will change
class EventsParser : public Parser {
	int length;
	std::map<std::string, std::map<int, Event*> > events; // events filtered by site, then by id

public:
	EventsParser(std::string eventsJson);
	int getLength();
	std::vector<Event*> findEventsBySite(std::string site);
	std::vector<Event*> findEventsById(int id);
	Event* findEvent(int id, std::string site);
	std::vector<Event> getEventsList();
};


class Event : public Parser {
	int id;
	std::string title;
	std::string site;
	std::string siteName;
	std::string link;
	std::string updatedTime;

public:
	Event(std::string eventJson);
	int getId();
	std::string getTitle();
	std::string getSite();
	std::string getSiteName();
	std::string getLink();
	std::string getUpdatedTime();
	
};

#endif