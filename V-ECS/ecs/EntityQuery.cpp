#include "EntityQuery.h"
#include "World.h"

using namespace vecs;

void ComponentFilter::with(std::type_index component) {
	required.insert(component);
}

void ComponentFilter::with(std::vector<std::type_index> components) {
	required.insert(components.begin(), components.end());
}

void ComponentFilter::without(std::type_index component) {
	disallowed.insert(component);
}

void ComponentFilter::without(std::vector<std::type_index> components) {
	disallowed.insert(components.begin(), components.end());
}
