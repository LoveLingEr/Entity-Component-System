#Entity-Component-System

## 示例

```cpp
#include	"Entity.h"
#include	<string>
#include	<iostream>

struct Position {
	float x, y, z;
	Position(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

struct Name {
	std::string name;
	Name(const std::string & name_) : name(name_) {}
	~Name() { std::cout << "Delete : " << name << std::endl; }
};

struct DemoSystem : public ISystem<Position, Name> {
	virtual void OnUpdate(float delta, Entity * entity, Position * p, Name * n) override {
		std::cout << "[P&N BY DEMOSYSTEM]" << entity->Id() << ". Name : " << n->name << ". POS : " << p->x << "," << p->y << ", " << p->z << std::endl;
	}
};

int main() {
	EntityManager manager;

	for (int i = 0; i < 10; ++i) {
		Entity * entity = manager.Create();
		entity->Add<Position>((float)i, 2.f, 3.f);
	}

	for (int i = 10; i < 20; ++i) {
		Entity * entity = manager.Create();
		entity->Add<Name>("Demo_" + std::to_string(i));
	}

	for (int i = 20; i < 30; ++i) {
		Entity * entity = manager.Create();
		entity->Add<Name>("Demo_" + std::to_string(i));
		entity->Add<Position>((float)i, 2.f, 3.f);
	}

	manager.Each<Position>([](Entity * entity, Position * p) {
		std::cout << "[POS]" << entity->Id() << ". Position : " << p->x << "," << p->y << ", " << p->z << std::endl;
	});

	manager.Each<Name>([](Entity * entity, Name * name) {
		std::cout << "[NAME]" << entity->Id() << ". Name : " << name->name << std::endl;
	});

	manager.Each<Position, Name>([](Entity * entity, Position * p, Name * name) {
		std::cout << "[P&N]" << entity->Id() << ". Name : " << name->name << ". POS : " << p->x << "," << p->y << ", " << p->z << std::endl;
	});

	{
		DemoSystem system;
		system.Update(&manager, 0);
	}

	for (int i = 21; i <= 30; ++i) {
		Entity * p = manager.Find(i);
		if (p) p->Destroy();
	}

	return 0;
}
```
