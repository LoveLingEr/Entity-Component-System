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

template<class E>
int IReceiver<E>::Filter() const {
	return TypeOf::Event<E>();
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

template<class R>
void EntityManager::Receiver(R * listener) {
	int type = listener->Filter();
	auto & set = _listeners[type];
	set.insert(listener);
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