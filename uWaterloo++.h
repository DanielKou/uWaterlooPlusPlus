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
class CoursesParser;
class Course;
class SpecificCourse;

class uWaterloo {
	CURL *curl;
	CURLcode res;
	const std::string apiKey;
	std::string result;
public:
	uWaterloo(std::string key);
	bool init();
	bool cleanUp();
	EventsParser parseEvents(); // returns parser for Events endpoint
	CoursesParser parseCourses();
};


// base parser class, defines base functionality
class Parser {
	const std::string raw;
protected:
	const std::string apiKey;
	json parser;
public:
	Parser(std::string json, std::string apiKey);
	std::string getRaw(); // gets raw json
	friend std::ostream& operator<<(std::ostream& out, Parser &p); // overloaded output stream operator
};


// NOTE: are copy constructor + assignment operator needed? shallow copy is ok, since all info is read only,
// nothing will change
class EventsParser : public Parser {
	int size;
	std::map<std::string, std::map<int, Event*> > events; // events filtered by site, then by id
public:
	EventsParser(std::string eventsJson, std::string apiKey);
	int getSize(); // returns number of events, which is size of getEventsList()
	std::vector<Event*> findEventsBySite(std::string site); // returns list of events at that site
	std::vector<Event*> findEventsById(int id); // returns list of events with that id
	Event* findEvent(int id, std::string site); // returns a pointer to the event or null
	std::vector<Event> getEventsList(); // returns all events
	std::vector<std::pair<std::string, std::string> > getHolidays(); // returns list of holidays in format: <name, date>
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
	SpecificEvent getMoreDetails(); // get more info for this event
	// getters
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
	// getters
	std::string getDescription();
	std::string getDescriptionRaw();
	double getCost();
	std::vector<std::pair<std::string, std::string> > getTimes();
	std::string getType();
	Location getLocation();
};


class CoursesParser : public Parser {
	int size;
	std::map<std::string, std::map<std::string, Course*> > courses_subject; // filtered by subject, then by catalog
	std::map<std::string, Course*> courses_id; // filtered by id
public:
	CoursesParser(std::string coursesJson, std::string apiKey);
	int getSize();
	Course* findCourse(std::string id); // returns pointer to course or null (search by id)
	Course* findCourse(std::string subject, std::string catalog); // returns pointer to course or null (search by subject and catalog)
	std::vector<Course*> findCoursesBySubject(std::string subject); // returns list of courses of that subject
	std::vector<Course> getCourseList(); // returns list of courses

};


class Course : public Parser {
	  std::string id;
	  std::string subject;
	  std::string catalog;
	  std::string title;
public:
	Course(std::string courseJson, std::string apiKey);
	SpecificCourse getMoreDetails(); // get more info for this course
	// getters
	std::string getId();
	std::string getSubjectAndCatalog(); // eg. CS 135
	std::string getTitle();
};



class SpecificCourse : public Course {
	double units;
	std::string description;
	std::string termsOffered; // F,W,S comma delimited
	std::string instructions; // LEC,LAB,TUT,TST comma delimited
	std::string url;
	std::string academicLevel;
public:
	SpecificCourse(std::string courseJson, std::string apiKey);
	bool isOffered(std::string term); // checks whether course is offered that term (search by whole word or letter: Winter/W)
	// getters
	std::string getDescription();
	std::string getTermsOffered();
	std::string getInstructionTypes();
	std::string getUrl();
	std::string getAcademicLevel();
};










#endif