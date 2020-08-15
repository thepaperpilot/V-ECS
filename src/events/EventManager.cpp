#include "EventManager.h"

using namespace vecs;

std::unordered_map<std::type_index, std::vector<std::function<bool(EventData*)>>> EventManager::listeners;
