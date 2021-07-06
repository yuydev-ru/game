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
    sf::Vector2f scale = {1, 1};
};
struct Player : Component {};

struct Enemy : Component {};

/* Systems */


void
render(GameState *state, Storage *storage, const Entity id)
{
    auto camera = storage->getComponent<Camera>(state->currentCamera);
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
    sf::Vector2f screenPos = { (t->position.x - camPos.x) * camera->scale.x
                             , (camPos.y - t->position.y) * camera->scale.y };

    screenPos += (sf::Vector2f) state->window->getSize() * 0.5f;

    auto spriteSize = (sf::Vector2f) spr->texture.getSize();
    spriteSize.x *= t->scale.x * camera->scale.x;
    spriteSize.y *= t->scale.y * camera->scale.y;
    screenPos -= spriteSize * 0.5f;

    auto screenScale = t->scale;
    screenScale.x *= camera->scale.x;
    screenScale.y *= camera->scale.y;

    spr->sprite.setPosition(screenPos);
    spr->sprite.setScale(screenScale);
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
// Функция переводит объект в заданную точку
void
stayEnemy(GameState *state, Storage *storage, const Entity id)
{
//    std::cout << id << std::endl;
    auto t = storage->getComponent<Transform>(id);

    t->position.x = 60.0f;
    t->position.y = 200.0f;

}
/* ******* */

//TODO: добавить функцию взаимодействия объектов, добавить булевскую функцию, выдающую true при коллизии
// (короче дохуя нам надо всего сделать еще...)

// NOTE(guschin): Возможно, эту функцию можно генерировать автоматически.
void
initializeEngine(GameState *state, Storage *storage)
{
    storage->registerComponent<Transform>();
    storage->registerComponent<Sprite>();
    storage->registerComponent<Camera>();
    storage->registerComponent<Player>();

    storage->registerComponent<Enemy>();

    storage->registerSystem(render, {TYPE(Transform), TYPE(Sprite)});
    storage->registerSystem(movePlayer, {TYPE(Transform), TYPE(Player)});
    storage->registerSystem(stayEnemy, {TYPE(Transform), TYPE(Enemy)});

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
// NOTE(tokarev): Создали структуру новый объект типа Enemy
    Entity e2 = storage->createEntity();
    auto e2_t = storage->addComponent<Transform>(e2);
    e2_t->scale = {0.1f, 0.1f};
    auto spr1 = storage->addComponent<Sprite>(e2);
    spr1->assetPath = "assets/images/enemy_cube.jpg";
    storage->addComponent<Enemy>(e2);


    Entity camera = storage->createEntity();
    storage->addComponent<Transform>(camera);
    auto cam = storage->addComponent<Camera>(camera);
    cam->scale = {1, 1};
    state->currentCamera = camera;
}