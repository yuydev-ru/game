#include <engine/interface.h>
#include <engine/base.h>

#include <iostream>

/* Components */

struct Transform : Component
{
    sf::Vector2f position = {0, 0};
    sf::Vector2f scale = {1, 1};
};
struct Sprite : Component
{
    std::string assetPath;
    bool loaded = false;
    sf::Image image;
    sf::Texture texture;
    sf::Sprite sprite;
};
struct Camera : Component
{

};
struct Player : Component {};


/* Systems */

void
render(GameState *state, Storage *storage, const Entity id)
{
    auto camTransform = storage->getComponent<Transform>(state->currentCamera);
    auto camPos = camTransform->position;

    auto t = storage->getComponent<Transform>(id);
    auto spr = storage->getComponent<Sprite>(id);

    if (!spr->loaded)
    {
        spr->image.loadFromFile(spr->assetPath);
        spr->texture.loadFromImage(spr->image);
        spr->sprite.setTexture(spr->texture);
        sf::IntRect rect = { 0, 0
                , (int) spr->image.getSize().x
                , (int) spr->image.getSize().y };
        spr->sprite.setTextureRect(rect);
        spr->loaded = true;
    }

    spr->sprite.setPosition(t->position);
    spr->sprite.setScale(t->scale);
    state->window->draw(spr->sprite);
}

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
    storage->registerComponent<Sprite>();
    storage->registerComponent<Camera>();
    storage->registerComponent<Player>();

    storage->registerSystem(render, {TYPE(Transform), TYPE(Sprite)});
    storage->registerSystem(movePlayer, {TYPE(Transform), TYPE(Player)});
}

// NOTE(guschin): В этой сцене должна загружаться указанная сцена, но
//  парсинга когфигов пока что нет, поэтому пока что так.
void
loadScene(const Config *config, const std::string& sceneName, GameState *state, Storage *storage)
{
    Entity e1 = storage->createEntity();
    auto e1_t = storage->addComponent<Transform>(e1);
    e1_t->scale = {0.1f, 0.1f};
    auto spr = storage->addComponent<Sprite>(e1);
    spr->assetPath = "assets/images/cube.jpg";
    storage->addComponent<Player>(e1);

    Entity camera = storage->createEntity();
    storage->addComponent<Transform>(camera);
    storage->addComponent<Camera>(camera);
    state->currentCamera = camera;
}