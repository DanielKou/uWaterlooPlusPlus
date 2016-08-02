#include "uWaterloo++.h"

using namespace std;

int main() {
	uWaterloo api("82873b516c87479fc4f3dc82f5fb20f2");
	api.init();
	EventsParser p = api.parseEvents();

	Event e = *(p.findEvent(447, "recreation-and-leisure-studies"));
	cout << e;

	api.cleanUp();

}