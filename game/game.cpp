#include <engine/interface.h>
#include <engine/base.h>

#include <set>
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
struct Collider : Component
{
    float width = 0;
    float height = 0;
    sf::Vector2f deltaCenter = {0, 0};
    sf::Vector2f leftDownCorner = {0, 0};
    sf::Vector2f rightUpCorner = {0, 0};
    std::set<unsigned int> collisionList;
};

/* Systems */

void
render(GameState *state, Storage *storage, const Entity id)
{
    auto camera = storage->getComponent<Camera>(state->currentCamera);
    auto camTransform = storage->getComponent<Transform>(state->currentCamera);
    auto camPos = camTransform->position;

    auto t = storage->getComponent<Transform>(id);
    auto spr = storage->getComponent<Sprite>(id);

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
updateCollider(GameState *state, Storage *storage, const Entity id)
{
    for (Entity id : storage->usedIds)
    {
        auto coll = storage->getComponent<Collider>(id);
        if (coll != nullptr)
        {
            auto t = storage->getComponent<Transform>(id);
            auto tPos = t->position;
            auto tSc = t->scale;
            auto dc = coll->deltaCenter;
            float w = coll->width * 0.5 * tSc.x;
            float h = coll->height * 0.5 * tSc.y;
            // NOTE(Roma) : Вычисление размеров коллайдера относительно центра коллайдера, его длины и ширины
            coll->leftDownCorner = {tPos.x + dc.x - w, tPos.y + dc.y - h};
            coll->rightUpCorner = {tPos.x + dc.x + w, tPos.y + dc.y + h};
        }
    }
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

void
collision (GameState *state, Storage *storage, const Entity id)
{
    auto c = storage->getComponent<Collider>(id);
    auto lC = c->leftDownCorner;
    auto rC = c->rightUpCorner;
    for (Entity id2 : storage->usedIds)
    {
        auto c2 = storage->getComponent<Collider>(id2);
        if (c2 != nullptr && id2 != id)
        {
            auto lC2 = c2->leftDownCorner;
            auto rC2 = c2->rightUpCorner;
            if (rC.x < lC2.x || lC.x > rC2.x || rC.y < lC2.y || lC.y > rC2.y)
            {
                c->collisionList.erase(id2);
                c2->collisionList.erase(id);
                continue;
            }
            c->collisionList.insert(id2);
            c2->collisionList.insert(id);

        }
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
    storage->registerComponent<Collider>();

    storage->registerSystem(render, {TYPE(Transform), TYPE(Sprite)});
    storage->registerSystem(movePlayer, {TYPE(Transform), TYPE(Player)});
    storage->registerSystem(updateCollider, {TYPE(Transform), TYPE(Sprite)});
    storage->registerSystem(collision, {TYPE(Collider), TYPE(Player)});

}

// NOTE(guschin): В этой сцене должна загружаться указанная сцена, но
//  парсинга когфигов пока что нет, поэтому пока что так.
void
loadScene(const Config *config, const std::string& sceneName, GameState *state, Storage *storage)
{
    Entity e1 = storage->createEntity();
    auto e1_t = storage->addComponent<Transform>(e1);
    storage->addComponent<Player>(e1);
    e1_t->scale = {0.1f, 0.1f};
    auto spr = storage->addComponent<Sprite>(e1);
    spr->assetPath = "assets/images/cube.jpg";

    Entity e2 = storage->createEntity();
    auto e2_t = storage->addComponent<Transform>(e2);
    e2_t->scale = {0.15f, 0.15f};
    auto spr2 = storage->addComponent<Sprite>(e2);
    spr2->assetPath = "assets/images/cube.jpg";
    e2_t->position = {130.f, 130.f};

    for (Entity ent_id : storage->usedIds)
    {
        auto spr = storage->getComponent<Sprite>(ent_id);
        if (spr != nullptr)
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
    }
    auto c = storage->addComponent<Collider>(e1);
    auto c2 = storage->addComponent<Collider>(e2);

    c->deltaCenter = {-30.0, -30.0};
    c2->deltaCenter = {1.0, 1.0};
    c->width = spr->image.getSize().x;
    c->height = spr->image.getSize().y;
    c2->width = spr2->image.getSize().x;
    c2->height = spr2->image.getSize().y;

    Entity camera = storage->createEntity();
    storage->addComponent<Transform>(camera);
    auto cam = storage->addComponent<Camera>(camera);
    cam->scale = {1, 1};
    state->currentCamera = camera;
}