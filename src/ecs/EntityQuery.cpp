#include "EntityQuery.h"
#include "World.h"

using namespace vecs;

void ComponentFilter::with(std::string component) {
	required.insert(component);
}

void ComponentFilter::with(std::vector<std::string> components) {
	required.insert(components.begin(), components.end());
}

void ComponentFilter::without(std::string component) {
	disallowed.insert(component);
}

void ComponentFilter::without(std::vector<std::string> components) {
	disallowed.insert(components.begin(), components.end());
}
