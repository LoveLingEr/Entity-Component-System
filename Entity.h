#ifndef		__ENTITY_H_INCLUDED__
#define		__ENTITY_H_INCLUDED__

#include	<bitset>
#include	<cassert>
#include	<cstdint>
#include	<cstdlib>
#include	<cstring>
#include	<functional>
#include	<map>
#include	<set>
#include	<vector>

#define		COMPONENT_MAX_TYPE	64
#define		EVENT_MAX_TYPE		64

struct IAllocator { virtual void Free(void *) = 0; };
typedef std::bitset<COMPONENT_MAX_TYPE> Mask;

/**
 * Declarations.
 */
class Entity;
class EntityManager;

/**
 * Entity definition for Entity-Component-System framework.
 */
class Entity {
public:
	Entity(EntityManager * manager, uint32_t id) : _manager(manager), _id(id), _mask() {}

	/**
	 * Disable delete operations on Entity. You should use \ref Entity::Destroy instead.
	 */
	void operator delete(void *) = delete;
	void operator delete[](void *) = delete;

	/**
	 * Get unique identifier.
	 */
	inline uint32_t Id() const { return _id; }

	/**
	 * Get owner manager.
	 */
	inline EntityManager * Owner() const { return _manager; }

	/**
	 * Test if this entity has all the required components.
	 *
	 * \param	mask	Component mask.
	 */
	bool Test(Mask & mask) const;

	/**
	 * Test if there exists a component as given type attached to this entity.
	 */
	template<class C>
	bool Has();

	/**
	 * Attach a component to this entity.
	 *
	 * \param	...	Parameters to construct this component.
	 * \return	Pointer of component just added.
	 */
	template<class C, typename ... Args>
	C * Add(Args ... args);

	/**
	 * Get component of type attached to this entity.
	 */
	template<class C>
	C * Get();

	/**
	 * Remove component of type attached to this entity.
	 */
	template<class C>
	void Delete();

	/**
	 * Destroy this entity. Release all component attached to it.
	 */
	void Destroy();

private:
	EntityManager *	_manager;
	uint32_t		_id;
	Mask			_mask;
};

/**
 * System for Entity-Component-System framework.
 */
template<class ... Required>
struct ISystem {
	/**
	 * Delta time post by Update.
	 */
	float delta;

	/**
	 * Current entity manager. DO NOT use this out of OnUpdate.
	 */
	EntityManager * manager;

	/**
	 * Invoke process entities with required components.
	 *
	 * \param	manager	Entity manager hold entities to be query.
	 * \param	delta	Delta time between last update.
	 */
	void Update(EntityManager * manager, float delta);

	/**
	 * Invoke process special entity with required components.
	 *
	 * \param	entity	Single entity you want to process.
	 * \param	delta	Delta time.
	 */
	void Update(Entity * entity, float delta);

	/**
	 * [Callback of Update] Process entity with required components.
	 *
	 * \param	entity	Entity found with required components.
	 * \param	...		Components attached to this entity.
	 */
	virtual void OnUpdate(Entity * entity, Required * ...) = 0;
};

/**
* Event receiver
*/
template<class E>
struct IReceiver {
	/**
	* Interface for receive event.
	*
	* \param	ev	Event data.
	*/
	virtual void OnEvent(E & ev) = 0;
};

/**
 * Manage entities & components.
 */
class EntityManager {
	struct Block {
		Entity	entity;
		bool	valid;
		void *	components[COMPONENT_MAX_TYPE];

		Block(EntityManager * manager, uint32_t id) : entity(manager, id), valid(true), components() {}
	};

public:
	EntityManager();
	virtual ~EntityManager();

	/**
	 * Alloc an entity.
	 */
	Entity * Create();

	/**
	 * Find an entity by unique identifier.
	 *
	 * \param	id	Entity's identifier \ref Entity::Id().
	 * \return	Pointer to this entity.
	 */
	Entity * Find(uint32_t id);

	/**
	 * Destroy an entity.
	 *
	 * \param	entity	Entity to be destroyed.
	 */
	void Destroy(Entity * entity);

	/**
	 * Create a new component & attach it to some entity.
	 *
	 * \param	entity	Attach to which entity.
	 * \param	...		Parameters to create this component.
	 * \return	Added component.
	 */
	template<class C, typename ... Args>
	C * AddComponent(Entity * entity, Args ... args);

	/**
	 * Get special type of component from some entity.
	 *
	 * \param	entity	Which entity to search.
	 * \return	Component pointer.
	 */
	template<class C>
	C * GetComponent(Entity * entity);

	/**
	 * Delete special component from some entity.
	 *
	 * \param	entity	Entity instance that hold this component.
	 */
	template<class C>
	void DeleteComponent(Entity * entity);

	/**
	 * Query entity has all given type of components.
	 *
	 * \param f	Callback to deal with those entities. Type : void(Entity *, Required *...)
	 */
	template<class ... Required, class F>
	void Each(F f);

	/**
	 * Query all entities.
	 */
	void Traverse(std::function<void(Entity *)> f);

	/**
	 * Listen on given event type.
	 */
	template<class ... Events, class R>
	void Subscribe(R * listener);

	/**
	 * Raise an event.
	 *
	 * \param	...	Parameter for construct this event.
	 */
	template<class E, typename ... Args>
	void Raise(Args ... args);

private:
	void __BeginEach();
	void __EndEach();

private:
	uint32_t _allocated;
	int _depth;
	std::map<uint32_t, Block *> _entities;
	IAllocator * _entity_allocator;
	IAllocator * _component_allocator[COMPONENT_MAX_TYPE];
	std::vector<std::set<void *>> _listeners;
	std::vector<uint32_t> _invalids;
};

//////////////////////////////////////////////////////////////////////////
/// Implements all inline & template methods.
//////////////////////////////////////////////////////////////////////////
class TypeOf {
public:
	template<class C>
	static int	Component() {
		static int id = _components++;
		assert(id < COMPONENT_MAX_TYPE);
		return id;
	}

	template<class E>
	static int Event() {
		static int id = _events++;
		assert(id < EVENT_MAX_TYPE);
		return id;
	}

private:
	static int	_components;
	static int	_events;
};

template<int N, class Head, class ... Tail>
struct MaskGen {
	static void Set(Mask & mask) {
		mask.set(TypeOf::Component<Head>());
		MaskGen<N - 1, Tail...>::Set(mask);
	}
};

template<class Head, class ... Tail>
struct MaskGen<1, Head, Tail...> {
	static void Set(Mask & mask) {
		mask.set(TypeOf::Component<Head>());
	}
};

template<class ... Args>
struct MaskOf {
	static Mask Make() {
		Mask mask;
		MaskGen<sizeof...(Args), Args...>::Set(mask);
		return std::move(mask);
	}
};

template<class T>
class Allocator : public IAllocator {
	struct Chunk {
		Chunk *	pNext;
		bool	bUsed;
		T *		pData;

		inline void Init() {
			pNext = nullptr;
			bUsed = false;
			pData = (T *)((char *)this + sizeof(Chunk));
		}
	};

public:
	Allocator(size_t nStep) : _nStep(nStep), _pHead(new Chunk) {
		_pHead->Init();
		__Extend();
	}

	virtual ~Allocator() {
		Clear();
		for (auto pMem : _lMem) delete[] pMem;
		_lMem.clear();
	}

	virtual void Free(void * p) override {
		if (!p) return;
		Free((T *)p);
	}

	template<typename ... Args>
	T *	Alloc(Args ... args) {
		if (!_pHead->pNext && !__Extend()) return nullptr;

		Chunk * pNode = _pHead->pNext;
		_pHead->pNext = _pHead->pNext->pNext;
		pNode->bUsed = true;
		return new (pNode->pData) T(args...);
	}

	void Free(T * pObj) {
		if (!pObj) return;
		pObj->~T();
		memset(pObj, 0, sizeof(T));

		Chunk * pNode = (Chunk *)((char *)pObj - sizeof(Chunk));
		pNode->bUsed = false;
		pNode->pNext = _pHead->pNext;
		_pHead->pNext = pNode;
	}

	void Clear() {
		size_t nBlock = sizeof(Chunk) + sizeof(T);
		for (auto pMem : _lMem) {
			for (size_t nLoop = 0; nLoop < _nStep; ++nLoop) {
				Chunk * pCur = (Chunk *)(pMem + nLoop * nBlock);
				if (pCur->bUsed) {
					pCur->bUsed = false;
					pCur->pData->~T();
					memset(pCur->pData, 0, sizeof(T));

					pCur->pNext = _pHead->pNext;
					_pHead->pNext = pCur;
				}
			}
		}
	}

private:
	bool __Extend() {
		size_t nBlock = sizeof(Chunk) + sizeof(T);
		char * pMem = new char[nBlock * _nStep];
		if (!pMem) return false;

		char * pCur = pMem;
		for (size_t nLoop = 0; nLoop < _nStep; ++nLoop, pCur += nBlock) {
			Chunk * pNode = (Chunk *)(pCur);
			pNode->Init();
			pNode->pNext = _pHead->pNext;
			_pHead->pNext = pNode;
		}

		_lMem.push_back(pMem);
		return true;
	}

private:
	size_t				_nStep;
	Chunk *				_pHead;
	std::vector<char *>	_lMem;
};

template<class C>
bool Entity::Has() {
	bool * valid = (bool *)(((char *)this) + sizeof(Entity));
	if (!(*valid)) return false;
	return _mask.test(TypeOf::Component<C>());
}

inline bool Entity::Test(Mask & mask) const {
	bool * valid = (bool *)(((char *)this) + sizeof(Entity));
	if (!(*valid)) return false;
	return (mask & _mask) == mask;
}

template<class C, typename ... Args>
C * Entity::Add(Args ... args) {
	int type = TypeOf::Component<C>();
	if (_mask.test(type)) _manager->DeleteComponent<C>(this);
	C * component = _manager->AddComponent<C>(this, args...);
	if (component) _mask.set(type);
	return component;
}

template<class C>
C * Entity::Get() {
	return _manager->GetComponent<C>(this);
}

template<class C>
void Entity::Delete() {
	_manager->DeleteComponent<C>(this);
	_mask.set(TypeOf::Component<C>(), false);
}

template<class ... Required>
void ISystem<Required...>::Update(EntityManager * manager, float delta) {
	this->delta = delta;
	this->manager = manager;
	manager->Each<Required...>([this](Entity * entity, Required * ... args) {
		this->OnUpdate(entity, args...);
	});
	this->manager = nullptr;
}

template<class ... Required>
void ISystem<Required...>::Update(Entity * entity, float delta) {
	Mask mask = MaskOf<Required...>::Make();
	if (entity->Test(mask)) {
		this->delta = delta;
		this->manager = entity->Owner();
		this->OnUpdate(entity, entity->Get<Required>() ...);
	}
	this->manager = nullptr;
}

template<class C, typename ... Args>
C * EntityManager::AddComponent(Entity * entity, Args ... args) {
	Block * p = (Block *)entity;
	if (!p->valid) return nullptr;

	int type = TypeOf::Component<C>();
	Allocator<C> * allocator = (Allocator<C> *)_component_allocator[type];
	if (!allocator) {
		allocator = new Allocator<C>(16);
		_component_allocator[type] = allocator;
	}

	C * component = allocator->Alloc(args...);
	if (component) p->components[type] = component;
	return component;
}

template<class C>
C * EntityManager::GetComponent(Entity * entity) {
	int type = TypeOf::Component<C>();
	Block * p = (Block *)entity;
	if (!p->valid) return nullptr;
	return (C *)p->components[type];
}

template<class C>
void EntityManager::DeleteComponent(Entity * entity) {
	Block * p = (Block *)entity;
	if (!p->valid) return;

	int type = TypeOf::Component<C>();
	if (p->components[type]) _component_allocator[type]->Free(p);
}

template<class ... Required, class F>
void EntityManager::Each(F f) {
	__BeginEach();

	Mask mask = MaskOf<Required...>::Make();
	for (auto & kv : _entities) {
		Entity * p = (Entity *)kv.second;
		if (p->Test(mask)) f(p, (Required *)((Block *)p)->components[TypeOf::Component<Required>()] ...);
	}

	__EndEach();
}

template<class ... Events, class R>
void EntityManager::Subscribe(R * listener) {
	int _[] = { (_listeners[TypeOf::Event<Events>()].insert((IReceiver<Events> *)listener), 0) ... };
}

template<class E, typename ... Args>
void EntityManager::Raise(Args ... args) {
	int type = TypeOf::Event<E>();
	E ev(args...);

	for (void * p : _listeners[type]) {
		IReceiver<E> * listener = (IReceiver<E> *)p;
		listener->OnEvent(ev);
	}
}

#endif//!	__ENTITY_H_INCLUDED__
