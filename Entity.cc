#include	"Entity.h"

int TypeOf::_counter = 0;

void Entity::Destroy() {
	_manager->Destroy(this);
}

EntityManager::EntityManager()
	: _allocated(0)
	, _depth(0)
	, _entities()
	, _entity_allocator(new Allocator<Block>(16))
	, _component_allocator()
	, _invalids() {
}

EntityManager::~EntityManager() {
	if (_entity_allocator) delete _entity_allocator;
	for (int i = 0; i < COMPONENT_MAX_TYPE; ++i) {
		if (_component_allocator[i]) delete _component_allocator[i];
	}
}

Entity * EntityManager::Create() {
	_allocated++;
	uint32_t id = _allocated;
	Block * p = ((Allocator<Block> *)_entity_allocator)->Alloc(this, id);
	if (!p) return nullptr;
	_entities[id] = p;
	return &p->entity;
}

Entity * EntityManager::Find(uint32_t id) {
	auto it = _entities.find(id);
	if (it == _entities.end()) return nullptr;
	return &it->second->entity;
}

void EntityManager::Destroy(Entity * entity) {
	if (!entity) return;

	Block * p = (Block *)entity;
	if (_depth > 0) {
		p->valid = false;
		_invalids.push_back(entity->Id());
		return;
	}
	
	for (int i = 0; i < COMPONENT_MAX_TYPE; ++i) {
		if (p->components[i]) _component_allocator[i]->Free(p->components[i]);
	}

	_entity_allocator->Free(p);
}

void EntityManager::__BeginEach() {
	_depth++;
}

void EntityManager::__EndEach() {
	_depth--;
	
	if (_depth > 0) return;
	_depth = 0;

	for (size_t i = 0; i < _invalids.size(); ++i) {
		Block * p = _entities[_invalids[i]];
		for (int j = 0; j < COMPONENT_MAX_TYPE; ++j) {
			if (p->components[i]) _component_allocator[i]->Free(p->components[i]);
		}
		_invalids.clear();
		_entity_allocator->Free(p);
	}
}
