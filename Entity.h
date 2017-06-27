#ifndef		__ENTITY_H_INCLUDED__
#define		__ENTITY_H_INCLUDED__

#include	<bitset>
#include	<cassert>
#include	<cstdint>
#include	<cstdlib>
#include	<cstring>
#include	<map>
#include	<set>
#include	<vector>

#define		COMPONENT_MAX_TYPE	64

/**
 * Generate allocator
 */
struct IAllocator {
	virtual void Free(void *) = 0;
};

/**
 * Component having status mask.
 */
typedef std::bitset<COMPONENT_MAX_TYPE> Mask;

/**
 * Entity definition for Entity-Component-System framework.
 */
class EntityManager;
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
	inline bool Test(Mask & mask) const { return (_mask & mask) == mask; }

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
	 * Invoke process entities with required components.
	 *
	 * \param	manager	Entity manager hold entities to be query.
	 * \param	delta	Delta time between last update.
	 */
	void Update(EntityManager * manager, float delta);

	/**
	 * Invoke proccess special entity with required components.
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

private:
	void __BeginEach();
	void __EndEach();

private:
	uint32_t _allocated;
	int _depth;
	std::map<uint32_t, Block *> _entities;
	IAllocator * _entity_allocator;
	IAllocator * _component_allocator[COMPONENT_MAX_TYPE];
	std::vector<uint32_t> _invalids;
};

#include	"Entity.inl"

#endif//!	__ENTITY_H_INCLUDED__
