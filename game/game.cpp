#include <engine/interface.h>
#include <engine/base.h>

#include <nlohmann/json.hpp>

#include <fstream>
#include <set>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

#include <typeindex>
#include <cmath>

using json = nlohmann::json;

namespace Parsing {
    static sf::Vector2f
    parseVector(json &dict, const std::string &key)
    {
        auto v = dict.find(key)->get<std::vector<float>>();
        return { v[0], v[1] };
    }
}


struct Transform : Component
{
    sf::Vector2f position = {0, 0};
    sf::Vector2f scale = {1, 1};

    static Component *
    deserialize(json &dict)
    {
        auto t = new Transform;

        t->position = Parsing::parseVector(dict, "position");

        auto scale = dict.find("scale")->get<std::vector<float>>();
        t->scale = { scale[0], scale[1] };

        return t;
    }
};

struct Sprite : Component
{
    std::string assetPath;
    bool loaded = false;
    sf::Image image;
    sf::Texture texture;
    sf::Sprite sprite;

    static Component *
    deserialize(json &dict)
    {
        auto s = new Sprite;

        auto assetPath = dict.find("assetPath")->get<std::string>();
        s->assetPath = assetPath;
        return s;
    }
};
struct Camera : Component
{
    sf::Vector2f scale = {1, 1};

    static Component *
    deserialize(json &dict)
    {
        auto c = new Camera;

        auto scale = dict.find("scale")->get<std::vector<float>>();
        c->scale = { scale[0], scale[1] };

        return c;
    }
};
struct Player : Component
{
    float speed = .5;

    static Component *
    deserialize(json &dict)
    {
        return new Player;
    }
};
struct Collider : Component
{
    float width = 0;
    float height = 0;
    sf::Vector2f deltaCenter = {0, 0};
    sf::Vector2f leftDownCorner = {0, 0};
    sf::Vector2f rightUpCorner = {0, 0};
    std::set<Entity> collisionList;

    static Component *
    deserialize(json &dict)
    {
        auto c = new Collider;
        c->deltaCenter = Parsing::parseVector(dict, "deltaCenter");
        c->width = dict.find("width")->get<int>();
        c->height = dict.find("height")->get<int>();
        return c;
    }
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
            float w = coll->width * 0.5f * tSc.x;
            float h = coll->height * 0.5f * tSc.y;
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

    sf::Vector2f move = {state->axes["horizontal"], state->axes["vertical"]};

    // Нормализуем верктор move
    float length = sqrt((move.x * move.x) + (move.y * move.y));
    if (length != 0)
    {
        move /= length;
    }

    move.x *= storage->getComponent<Player>(id)->speed;
    move.y *= storage->getComponent<Player>(id)->speed;

    t->position += move;
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
            std::cout << "Can't touch this!\n";
        }
    }
}

/* ******* */

// NOTE(guschin): Возможно, эту функцию можно генерировать автоматически.
void
initializeEngine(GameState *state, Storage *storage)
{

    storage->registerComponent<Transform>("Transform");
    storage->registerComponent<Sprite>("Sprite");
    storage->registerComponent<Camera>("Camera");
    storage->registerComponent<Player>("Player");
    storage->registerComponent<Collider>("Collider");
    storage->registerSystem(render, {TYPE(Transform), TYPE(Sprite)});
    storage->registerSystem(movePlayer, {TYPE(Transform), TYPE(Player)});
    storage->registerSystem(updateCollider, {TYPE(Transform), TYPE(Sprite)});
    storage->registerSystem(collision, {TYPE(Collider), TYPE(Player)});

}

Entity
loadEntity(json &components, GameState *state, Storage *storage)
{
    Entity entity = storage->createEntity();

    for (auto & component : components)
    {
        std::string name = component.find("type")->get<std::string>();
        if (storage->deserializers.find(name) != storage->deserializers.end())
        {
            Component *comp = storage->deserializers[name](component);
            auto type = storage->typeNames.at(name);
            storage->entities[type][entity] = comp;
            storage->entitySignatures[entity].set(storage->componentTypes[type]);

            if (name == "Camera")
            {
                state->currentCamera = entity;
            }
        }
    }

    return entity;
}

// NOTE(guschin): В этой сцене должна загружаться указанная сцена, но
//  парсинга когфигов пока что нет, поэтому пока что так.
void
loadScene(const Config *config, const std::string& sceneName, GameState *state, Storage *storage)
{
    std::ifstream ifs("assets/foo.json");
    auto j = json::parse(ifs);
    ifs.close();
    auto entities = j.find("entities");



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
    /*
    auto c = storage->addComponent<Collider>(e1);
    auto c2 = storage->addComponent<Collider>(e2);

    c->deltaCenter = {-30.0, -30.0};
    c2->deltaCenter = {1.0, 1.0};
    c->width = (float) spr->image.getSize().x;
    c->height = (float) spr->image.getSize().y;
    c2->width = (float) spr2->image.getSize().x;
    c2->height = (float) spr2->image.getSize().y;
*/
    for (auto components : *entities)
    {
        loadEntity(components, state, storage);
    }
}