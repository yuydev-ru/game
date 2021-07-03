#include <engine/interface.h>
#include <engine/base.h>

#include <iostream>

/* Components */

struct Transform : Component
{
    int x;
    int y;
};
struct PlayerController : Component
{
    std::string name;
};
struct Collider : Component {};
struct EnemyController : Component {};

/* Systems */

void
movePlayer(GameState *state, Storage *storage, const Entity id)
{
    auto t = storage->getComponent<Transform>(id);
    auto p = storage->getComponent<PlayerController>(id);

    std::cout << "Moving player " << p->name << " from "
              << "(" << t->x << "; " << t->y << ")";

    t->x *= 21;
    t->y *= 13;

    std::cout << " to (" << t->x << "; " << t->y << ")" << std::endl;
    state->running = false;
}

/* ******* */

// NOTE(guschin): Возможно, эту функцию можно генерировать автоматически.
void
initializeEngine(GameState *state, Storage *storage)
{
    storage->registerComponent<Transform>();
    storage->registerComponent<EnemyController>();
    storage->registerComponent<PlayerController>();
    storage->registerComponent<Collider>();

    auto comp = {TYPE(Transform), TYPE(PlayerController)};
    storage->systemSignature[movePlayer] = storage->createSignature(comp);
    storage->systemsArray.push_back(movePlayer);
}

// NOTE(guschin): В этой сцене должна загружаться указанная сцена, но
//  парсинга когфигов пока что нет, поэтому пока что так.
void
loadScene(const Config *config, const std::string& sceneName, GameState *state, Storage *storage)
{
    Entity e1 = storage->createEntity();

    Entity e2 = storage->createEntity();
    auto t = storage->addComponent<Transform>(e2);
    auto p = storage->addComponent<PlayerController>(e2);
    t->x = 1;
    t->y = 2;

    p->name = "Andrew";

    storage->destroyEntity(e1);
}