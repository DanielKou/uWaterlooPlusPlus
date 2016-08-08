#include "uWaterloo++.h"

// callback function that writes to string and returns size
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


// backup string parser for the json
std::string nullCheck(json& j) {
	return (j.is_null()) ? "" : j.get<std::string>();
}


// lowercase conversion
std::string toLower(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	return str;
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

CoursesParser uWaterloo::parseCourses() {
	std::string url = "https://api.uwaterloo.ca/v2/courses.json?key=" + apiKey;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	res = curl_easy_perform(curl);

	// temp fix: clean up response, sometimes it screws up and appends garbage value (maybe look into why it happens)
	size_t found = result.find_last_of("}");
	result = result.substr(0, found + 1);
	CoursesParser cp(result, apiKey);
	return cp;
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
	size = parser.size();

	// initialize events array
	for (json::iterator it = parser.begin(); it != parser.end(); ++it) {
		int id = (*it)["id"];
		std::string site = (*it)["site"];

		Event *e = new Event((*it).dump(), apiKey);
		events[site].insert(std::make_pair(id, e));
	}
}


int EventsParser::getSize() {
	return size;
}


// filter by site
std::vector<Event*> EventsParser::findEventsBySite(std::string site) {
	std::vector<Event*> v;
	
	if (events.find(site) != events.end()) {
		MapToVec(events[site], v);
	}
	return v;
}


// filter by id
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
	if (it != events.end()) {
		std::map<int, Event*>::iterator it2 = it->second.find(id);
		return (it2 != it->second.end()) ? it2->second : NULL;
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
		
		// initialize json object
		json tempParser;
		std::stringstream ss;
		ss << tempResult;
		ss >> tempParser;
		tempParser = tempParser["data"];

		for (json::iterator it = tempParser.begin(); it != tempParser.end(); ++it) {
			v.push_back(std::make_pair((*it)["name"].get<std::string>(), (*it)["date"].get<std::string>()));
		}
		// always cleanup
		curl_easy_cleanup(tempCurl);
	}
	return v;
}


//------------------------------ EVENT ------------------------------------------------
//-------------------------------------------------------------------------------------
Event::Event(std::string eventJson, std::string apiKey, bool specific /* = false */) : Parser(eventJson, apiKey){
	id = parser["id"];
	title = nullCheck(parser["title"]); 
	site = nullCheck((specific) ? parser["site_id"] : parser["site"]); // key differs in event and specific event... thanks waterloo api
	siteName = nullCheck(parser["site_name"]);
	link = nullCheck(parser["link"]);
	updatedTime = nullCheck(parser["updated"]);
}


SpecificEvent Event::getMoreDetails() {
	std::string tempResult;
	CURL* tempCurl = curl_easy_init();
	if (tempCurl) {
		curl_easy_setopt(tempCurl, CURLOPT_WRITEFUNCTION, writeCallback);
		curl_easy_setopt(tempCurl, CURLOPT_WRITEDATA, &tempResult);
		curl_easy_setopt(tempCurl, CURLOPT_URL, ("https://api.uwaterloo.ca/v2/events/" 
												+ site + std::string("/") + std::to_string(id) 
												+ std::string(".json?key=") + apiKey).c_str());
		curl_easy_perform(tempCurl);
		// always cleanup
		curl_easy_cleanup(tempCurl);

		// initialize json object
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
	description = nullCheck(parser["description"]);
	descriptionRaw = nullCheck(parser["description_raw"]);
	cost = (parser["cost"].is_number()) ? parser["cost"] : 0;

	// get list of times
	json temp = parser["times"];
	for (json::iterator it = temp.begin(); it != temp.end(); ++it) {
		times.push_back(std::make_pair(nullCheck((*it)["start"]), nullCheck((*it)["end"])));
	}

	// get list of types, save as comma seperated string
	temp = parser["type"];
	int size = temp.size();
	for (int i = 0; i < size; i++) {
		if (i != 0) {
			type += ",";
		}
		type += nullCheck(temp[i]);
	}

	temp = parser["location"];
	location.name = nullCheck(temp["name"]);
	location.street = nullCheck(temp["street"]);
	location.city = nullCheck(temp["city"]);
	location.province = nullCheck(temp["province"]);
	location.postalCode = nullCheck(temp["postal_code"]);
}


// getters
std::string SpecificEvent::getDescription() { return description; }
std::string SpecificEvent::getDescriptionRaw() { return descriptionRaw; }
double SpecificEvent::getCost() { return cost; }
std::vector<std::pair<std::string, std::string> > SpecificEvent::getTimes() { return times; }
std::string SpecificEvent::getType() { return type; }
Location SpecificEvent::getLocation() { return location; }



//------------------------------ COURSES PARSER ---------------------------------------
//-------------------------------------------------------------------------------------
CoursesParser::CoursesParser(std::string coursesJson, std::string apiKey) : Parser(coursesJson, apiKey) {
	parser = parser["data"];
	size = parser.size();

	for (json::iterator it = parser.begin(); it != parser.end(); ++it) {
		std::string id = (*it)["course_id"];
		std::string subject = (*it)["subject"];
		std::string catalog = (*it)["catalog_number"];

		Course *c = new Course((*it).dump(), apiKey);

		// filter courses
		courses_subject[subject].insert(std::make_pair(catalog, c));
		courses_id.insert(std::make_pair(id, c));
	}
}


int CoursesParser::getSize() {
	return size;
}


Course* CoursesParser::findCourse(std::string id) {
	std::map<std::string, Course*>::iterator it = courses_id.find(id);
	return (it != courses_id.end()) ? it->second : NULL;
}


Course*  CoursesParser::findCourse(std::string subject, std::string catalog) {
	std::map<std::string, std::map<std::string, Course*> >::iterator it = courses_subject.find(subject);
	if (it != courses_subject.end()) {
		std::map<std::string, Course*>::iterator it2 = it->second.find(catalog);
		return (it2 != it->second.end()) ? it2->second : NULL;
	}
	else {
		return NULL;
	}
}


std::vector<Course*> CoursesParser::findCoursesBySubject(std::string subject) {
	std::vector<Course*> c;

	if (courses_subject.find(subject) != courses_subject.end()) {
		MapToVec(courses_subject[subject], c);
	}
	return c;
}


std::vector<Course> CoursesParser::getCourseList() {
	std::vector<Course> c;
	for (std::map<std::string, Course*>::iterator it = courses_id.begin(); it != courses_id.end(); it++) {
		Course e = *(it->second);
		c.push_back(e);
	}
	return c;
}



//------------------------------ COURSE -----------------------------------------------
//-------------------------------------------------------------------------------------
Course::Course(std::string courseJson, std::string apiKey) : Parser(courseJson, apiKey) {
	id = nullCheck(parser["course_id"]);
	subject = nullCheck(parser["subject"]);
	catalog = nullCheck(parser["catalog_number"]);
	title = nullCheck(parser["title"]);
}


// getters
std::string Course::getId() { return id; }
std::string Course::getSubjectAndCatalog() { return subject + " " + catalog; }
std::string Course::getTitle() { return title; }


SpecificCourse Course::getMoreDetails() {
	std::string tempResult;
	CURL* tempCurl = curl_easy_init();
	if (tempCurl) {
		curl_easy_setopt(tempCurl, CURLOPT_WRITEFUNCTION, writeCallback);
		curl_easy_setopt(tempCurl, CURLOPT_WRITEDATA, &tempResult);
		curl_easy_setopt(tempCurl, CURLOPT_URL, ("https://api.uwaterloo.ca/v2/courses/" + id + std::string(".json?key=") + apiKey).c_str());
		curl_easy_perform(tempCurl);
		// always cleanup
		curl_easy_cleanup(tempCurl);

		// initialize json object
		json tempJson;
		std::stringstream ss;
		ss << tempResult;
		ss >> tempJson;
		SpecificCourse sc(tempJson["data"].dump(), apiKey);
		return sc;
	}
}







//------------------------------ SPECIFIC COURSE --------------------------------------
//-------------------------------------------------------------------------------------
SpecificCourse::SpecificCourse(std::string courseJson, std::string apiKey) : Course(courseJson, apiKey) {
	units = (parser["units"].is_number()) ? parser["units"] : 0;
	description = nullCheck(parser["description"]);

	int size = parser["terms_offered"].size();
	for (int i = 0; i < size; i++) {
		if (i != 0) {
			termsOffered += ",";
		}
		termsOffered += nullCheck(parser["terms_offered"][i]);
	}

	size = parser["instructions"].size();
	for (int i = 0; i < size; i++) {
		if (i != 0) {
			termsOffered += ",";
		}
		instructions += nullCheck(parser["instructions"][i]);
	}

	url = nullCheck(parser["url"]);
	academicLevel = nullCheck(parser["academic_level"]);
}


// getters
std::string SpecificCourse::getDescription() { return description; }
std::string SpecificCourse::getTermsOffered() { return termsOffered; }
std::string SpecificCourse::getInstructionTypes() { return instructions; }
std::string SpecificCourse::getUrl() { return url; }
std::string SpecificCourse::getAcademicLevel() { return academicLevel; }


bool SpecificCourse::isOffered(std::string term) {
	std::string findTerm;
	term = toLower(term);
	if (term == "fall" || term == "f") {
		findTerm = "F";
	}
	else if (term == "winter" || term == "w") {
		findTerm = "W";
	}
	else if (term == "spring" || term == "s") {
		findTerm = "S";
	}

	if (findTerm != "") {
		return (termsOffered.find(findTerm) != std::string::npos);
	}

	return false;
}