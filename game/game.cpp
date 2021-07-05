#include <engine/interface.h>
#include <engine/base.h>

#include <iostream>

/* Components */

struct Transform : Component
{
    sf::Vector2f position = {0, 0};
};
struct Player : Component {};

/* Systems */

void
movePlayer(GameState *state, Storage *storage, const Entity id)
{
    auto t = storage->getComponent<Transform>(id);

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
    {
        t->position.y += 0.1f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
    {
        t->position.y -= 0.1f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
    {
        t->position.x -= 0.1f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
    {
        t->position.x += 0.1f;
    }
}

/* ******* */

// NOTE(guschin): Возможно, эту функцию можно генерировать автоматически.
void
initializeEngine(GameState *state, Storage *storage)
{
    storage->registerComponent<Transform>();
    storage->registerComponent<Player>();

    storage->registerSystem(movePlayer, {TYPE(Transform), TYPE(Player)});
}

// NOTE(guschin): В этой сцене должна загружаться указанная сцена, но
//  парсинга когфигов пока что нет, поэтому пока что так.
void
loadScene(const Config *config, const std::string& sceneName, GameState *state, Storage *storage)
{
    Entity e1 = storage->createEntity();
    storage->addComponent<Transform>(e1);
    storage->addComponent<Player>(e1);
}