#include "MovementSystem.h"
#include "PositionComponent.h"
#include "VelocityComponent.h"

using namespace vecs;

void MovementSystem::init() {
	withVelocity.filter.with(typeid(PositionComponent));
	withVelocity.filter.with(typeid(VelocityComponent));
	world->addQuery(&withVelocity);
}

void MovementSystem::update() {
	for (auto archetype : withVelocity.matchingArchetypes) {
		std::vector<Component*>* positionComponents = archetype->getComponentList(typeid(PositionComponent));
		std::vector<Component*>* velocityComponents = archetype->getComponentList(typeid(VelocityComponent));

		for (size_t i = 0; i < positionComponents->size(); i++) {
			PositionComponent* position = static_cast<PositionComponent*>(positionComponents->at(i));
			VelocityComponent* velocity = static_cast<VelocityComponent*>(velocityComponents->at(i));

			position->position += velocity->velocity * (float)world->deltaTime;
		}
	}
}
