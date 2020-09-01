#include "World.h"

#include "Archetype.h"
#include "EntityQuery.h"
#include "WorldLoadStatus.h"
#include "../engine/Device.h"
#include "../engine/Engine.h"
#include "../events/EventManager.h"
#include "../jobs/Worker.h"

using namespace vecs;

World::World(Engine* engine, std::string filename, WorldLoadStatus* status, bool waitUntilLoaded) : worker(engine, this) {
	device = engine->device;
	this->status = status;
	status->currentStep = WORLD_LOAD_STEP_SETUP;

	size_t numThreads = std::max(1u, std::thread::hardware_concurrency() - 1);
	size_t availableQueues = engine->device->queueFamilyIndices.graphicsQueueCount;
	size_t desiredQueues = numThreads + 3; // 1 for engine and 2 for worlds
	// Ternary to prevent underflow
	uint32_t overlap = availableQueues > desiredQueues ? 0 : desiredQueues - availableQueues;
	uint32_t queueIndex = engine->jobManager.getQueueIndex(1 + engine->nextQueueIndex, overlap > 0 ? availableQueues : desiredQueues);
	worker.init(queueIndex, engine->jobManager.getQueueLock(queueIndex));
	worker.stealPersistent = false;
	engine->nextQueueIndex = !engine->nextQueueIndex;

	setupEvents();

	nodeEditorContext = imnodes::EditorContextCreate();

	if (!std::filesystem::exists(filename)) {
		Debugger::addLog(DEBUG_LEVEL_WARN, "[WORLD] Attempted to load world at \"" + filename + "\" but file does not exist.");
		return;
	}

	auto result = worker.lua.script_file(filename);
	if (result.valid()) {
		config = result;
	}
	else {
		sol::error err = result;
		Debugger::addLog(DEBUG_LEVEL_ERROR, "[WORLD] Attempted to load world at \"" + filename + "\" but lua parsing failed with error:\n[LUA] " + std::string(err.what()));
		return;
	}

	auto job = dependencyGraph.load(engine, &worker, config, status);
	if (waitUntilLoaded && !status->isCancelled) {
		while (job->unfinishedJobs > 0)
			worker.work();
	} else if (status->isCancelled) cleanup();
}

World::World(Engine* engine, sol::table worldConfig, WorldLoadStatus* status, bool waitUntilLoaded) : worker(engine, this) {
	device = engine->device;
	this->status = status;
	status->currentStep = WORLD_LOAD_STEP_SETUP;

	size_t numThreads = std::max(1u, std::thread::hardware_concurrency() - 1);
	size_t availableQueues = engine->device->queueFamilyIndices.graphicsQueueCount;
	size_t desiredQueues = numThreads + 3; // 1 for engine and 2 for worlds
	// Ternary to prevent underflow
	uint32_t overlap = availableQueues > desiredQueues ? 0 : desiredQueues - availableQueues;
	uint32_t queueIndex = engine->jobManager.getQueueIndex(1 + engine->nextQueueIndex, overlap > 0 ? availableQueues : desiredQueues);
	worker.init(queueIndex, engine->jobManager.getQueueLock(queueIndex));
	engine->nextQueueIndex = !engine->nextQueueIndex;

	setupEvents();

	nodeEditorContext = imnodes::EditorContextCreate();

	auto job = dependencyGraph.load(engine, &worker, worldConfig, status);
	if (waitUntilLoaded && !status->isCancelled) {
		while (job->unfinishedJobs > 0)
			worker.work();
	} else if (status->isCancelled) cleanup();
}

uint32_t World::createEntities(uint32_t amount) {
	uint32_t firstEntity = nextEntity.fetch_add(amount);
	return firstEntity;
}

uint32_t World::createEntity(LuaVal* components) {
	std::unordered_set<std::string> componentTypes;
	LuaVal::MapType componentMap = *std::get<LuaVal::MapType*>(components->value);
	for (auto kvp : componentMap) {
		if (kvp.first.type == LUA_TYPE_STRING)
			componentTypes.insert(std::get<std::string>(kvp.first.value));
	}
	Archetype* archetype = getArchetype(componentTypes);
	auto entity = archetype->createEntities(1);
	for (auto kvp : componentMap) {
		archetype->getComponentList(std::get<std::string>(kvp.first.value)).set(LuaVal((double)entity.first), kvp.second);
	}
	return entity.first;
}

Archetype* World::getArchetype(std::unordered_set<std::string> componentTypes, LuaVal* sharedComponents) {
	archetypesMutex.lock_shared();
	auto itr = std::find_if(archetypes.begin(), archetypes.end(), [&componentTypes, &sharedComponents](Archetype* archetype) {
		// Check if they have the same number of non-shared components
		if (archetype->componentTypes.size() != componentTypes.size()) return false;
		// Check if the non-shared components they have are the same
		if (archetype->componentTypes != componentTypes) return false;
		// Check if shared components are the same. If so there's nothing left to check so we can just return true
		if (archetype->sharedComponents == sharedComponents) return true;
		// Check if either archetype doesn't have shared components and the other does
		// Note we know they don't both equal nullptr because of the earlier check for them being the same
		if (archetype->sharedComponents == nullptr || sharedComponents == nullptr) return false;
		// Check if the shared components are the same
		return *archetype->sharedComponents == *sharedComponents;
	});

	if (itr == archetypes.end()) {
		archetypesMutex.unlock_shared();
		archetypesMutex.lock();
		// Create a new archetype
		Archetype* archetype = archetypes.emplace_back(new Archetype(this, componentTypes, sharedComponents));
		archetypesMutex.unlock();

		// Add it to any entity queries it matches
		for (auto query : queries) {
			if (archetype->checkQuery(query))
				query->matchingArchetypes.push_back(archetype);
		}

		return archetype;
	}
	archetypesMutex.unlock_shared();

	return *itr;
}

void World::addQuery(EntityQuery* query) {
	queries.push_back(query);

	// Check what archetypes match this query
	for (auto archetype : archetypes) {
		if (archetype->checkQuery(query))
			query->matchingArchetypes.push_back(archetype);
	}
}

void World::update(double deltaTime) {
	this->deltaTime = deltaTime;
	worker.resetFrame();

	dependencyGraph.execute(&worker);

	// Delete GLFW events
	mouseMoveEventArchetype->clearEntities();
	leftMousePressEventArchetype->clearEntities();
	leftMouseReleaseEventArchetype->clearEntities();
	rightMousePressEventArchetype->clearEntities();
	rightMouseReleaseEventArchetype->clearEntities();
	horizontalScrollEventArchetype->clearEntities();
	verticalScrollEventArchetype->clearEntities();
	keyPressEventArchetype->clearEntities();
	keyReleaseEventArchetype->clearEntities();
}

void World::windowRefresh(int imageCount) {
	dependencyGraph.windowRefresh(imageCount);
	worker.createInheritanceInfo();
}

void World::addBuffer(Buffer buffer) {
	buffersMutex.lock();
	buffers.emplace_back(buffer);
	buffersMutex.unlock();
}

void World::cleanup() {
	// Destroy our systems and sub-renderers
	dependencyGraph.cleanup();

	// Destroy our worker
	worker.cleanup();

	// Destroy our node editor context
	imnodes::EditorContextFree(nodeEditorContext);

	// Destroy any buffers created by this world
	for (auto buffer : buffers)
		device->cleanupBuffer(buffer);

	// Set a flag so our event listeners will stop running
	// (There is no EventManager::RemoveListener since index is not guaranteed and the
	//  listeners map doesn't store the callbacks directly so getting the index is nontrivial)
	isDisposed = false;
}

void World::setupEvents() {
	// Get our archetypes for each event
	mouseMoveEventArchetype = getArchetype({ "MouseMoveEvent" });
	leftMousePressEventArchetype = getArchetype({ "LeftMousePressEvent" });
	leftMouseReleaseEventArchetype = getArchetype({ "LeftMouseReleaseEvent" });
	rightMousePressEventArchetype = getArchetype({ "RightMousePressEvent" });
	rightMouseReleaseEventArchetype = getArchetype({ "RightMouseReleaseEvent" });
	horizontalScrollEventArchetype = getArchetype({ "HorizontalScrollEvent" });
	verticalScrollEventArchetype = getArchetype({ "VerticalScrollEvent" });
	keyPressEventArchetype = getArchetype({ "KeyPressEvent" });
	keyReleaseEventArchetype = getArchetype({ "KeyReleaseEvent" });
	windowResizeEventArchetype = getArchetype({ "WindowResizeEvent" });

	// Create listeners to create event entities
	EventManager::addListener(this, &World::mouseMoveEventCallback);
	EventManager::addListener(this, &World::leftMousePressEventCallback);
	EventManager::addListener(this, &World::leftMouseReleaseEventCallback);
	EventManager::addListener(this, &World::rightMousePressEventCallback);
	EventManager::addListener(this, &World::rightMouseReleaseEventCallback);
	EventManager::addListener(this, &World::horizontalScrollEventCallback);
	EventManager::addListener(this, &World::verticalScrollEventCallback);
	EventManager::addListener(this, &World::keyPressEventCallback);
	EventManager::addListener(this, &World::keyReleaseEventCallback);
	EventManager::addListener(this, &World::windowResizeEventCallback);
}

bool World::mouseMoveEventCallback(MouseMoveEvent* event) {
	if (isDisposed) return false;
	if (!isValid) return true;
	mouseMoveEventArchetype->getComponentList("MouseMoveEvent").set(
		(double)mouseMoveEventArchetype->createEntities(1).first,
		{ { (std::string)"xPos", event->xPos }, { (std::string)"yPos", event->yPos } }
	);
	return true;
}

bool World::leftMousePressEventCallback(LeftMousePressEvent* event) {
	if (isDisposed) return false;
	if (!isValid) return true;
	leftMousePressEventArchetype->getComponentList("LeftMousePressEvent").set(
		(double)leftMousePressEventArchetype->createEntities(1).first,
		{ { (std::string)"mods", (double)event->mods } }
	);
	return true;
}

bool World::leftMouseReleaseEventCallback(LeftMouseReleaseEvent* event) {
	if (isDisposed) return false;
	if (!isValid) return true;
	leftMouseReleaseEventArchetype->getComponentList("LeftMouseReleaseEvent").set(
		(double)leftMouseReleaseEventArchetype->createEntities(1).first,
		{ { (std::string)"mods", (double)event->mods } }
	);
	return true;
}

bool World::rightMousePressEventCallback(RightMousePressEvent* event) {
	if (isDisposed) return false;
	if (!isValid) return true;
	rightMousePressEventArchetype->getComponentList("RightMousePressEvent").set(
		(double)rightMousePressEventArchetype->createEntities(1).first,
		{ { (std::string)"mods", (double)event->mods } }
	);
	return true;
}

bool World::rightMouseReleaseEventCallback(RightMouseReleaseEvent* event) {
	if (isDisposed) return false;
	if (!isValid) return true;
	rightMouseReleaseEventArchetype->getComponentList("RightMouseReleaseEvent").set(
		(double)rightMouseReleaseEventArchetype->createEntities(1).first,
		{ { (std::string)"mods", (double)event->mods } }
	);
	return true;
}

bool World::horizontalScrollEventCallback(HorizontalScrollEvent* event) {
	if (isDisposed) return false;
	if (!isValid) return true;
	horizontalScrollEventArchetype->getComponentList("HorizontalScrollEvent").set(
		(double)horizontalScrollEventArchetype->createEntities(1).first,
		{ { (std::string)"xOffset", event->xOffset } }
	);
	return true;
}

bool World::verticalScrollEventCallback(VerticalScrollEvent* event) {
	if (isDisposed) return false;
	if (!isValid) return true;
	verticalScrollEventArchetype->getComponentList("VerticalScrollEvent").set(
		(double)verticalScrollEventArchetype->createEntities(1).first,
		{ { (std::string)"yOffset", event->yOffset } }
	);
	return true;
}

bool World::keyPressEventCallback(KeyPressEvent* event) {
	if (isDisposed) return false;
	if (!isValid) return true;
	keyPressEventArchetype->getComponentList("KeyPressEvent").set(
		(double)keyPressEventArchetype->createEntities(1).first,
		{ { (std::string)"key", (double)event->key }, { (std::string)"mods", (double)event->mods }, { (std::string)"scancode", (double)event->scancode } }
	);
	return true;
}

bool World::keyReleaseEventCallback(KeyReleaseEvent* event) {
	if (isDisposed) return false;
	if (!isValid) return true;
	keyReleaseEventArchetype->getComponentList("KeyReleaseEvent").set(
		(double)keyReleaseEventArchetype->createEntities(1).first,
		{ { (std::string)"key", (double)event->key }, { (std::string)"mods", (double)event->mods }, { (std::string)"scancode", (double)event->scancode } }
	);
	return true;
}

bool World::windowResizeEventCallback(WindowResizeEvent* event) {
	if (isDisposed) return false;
	if (!isValid) return true;
	windowResizeEventArchetype->getComponentList("WindowResizeEvent").set(
		(double)windowResizeEventArchetype->createEntities(1).first,
		{ { (std::string)"width", (double)event->width }, { (std::string)"height", (double)event->height } }
	);
	return true;
}
