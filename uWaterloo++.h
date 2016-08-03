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


// Helper classes:
struct Location {
	std::string name;
	std::string street;
	std::string city;
	std::string province;
	std::string postalCode;
};

// API Classes:
class EventsParser;
class Event;
class SpecificEvent;

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
	const std::string apiKey;
	json parser;

public:
	Parser(std::string json, std::string apiKey);
	std::string getRaw();
	friend std::ostream& operator<<(std::ostream& out, Parser &p);
};

// NOTE: are copy constructor + assignment operator needed? shallow copy is ok, since all info is read only,
// nothing will change
class EventsParser : public Parser {
	int length;
	std::map<std::string, std::map<int, Event*> > events; // events filtered by site, then by id

public:
	EventsParser(std::string eventsJson, std::string apiKey);
	int getLength();
	std::vector<Event*> findEventsBySite(std::string site);
	std::vector<Event*> findEventsById(int id);
	Event* findEvent(int id, std::string site);
	std::vector<Event> getEventsList();
	std::vector<std::pair<std::string, std::string> > getHolidays();
};


class Event : public Parser {
	int id;
	std::string title;
	std::string site;
	std::string siteName;
	std::string link;
	std::string updatedTime;

public:
	Event(std::string eventJson, std::string apiKey, bool specific = false);
	SpecificEvent getSpecifics();
	int getId();
	std::string getTitle();
	std::string getSite();
	std::string getSiteName();
	std::string getLink();
	std::string getUpdatedTime();
	
};


class SpecificEvent : public Event {
	std::string description;
	std::string descriptionRaw;
	double cost;
	std::vector<std::pair<std::string, std::string> > times; // start and end times
	std::string type; // comma delimited
	struct Location location;
public:
	SpecificEvent(std::string eventJson, std::string apiKey);
	std::string getDescription();
	std::string getDescriptionRaw();
	double getCost();
	std::vector<std::pair<std::string, std::string> > getTimes();
	std::string getType();
	Location getLocation();
};












#endif