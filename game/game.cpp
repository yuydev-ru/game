#include <engine/interface.h>
#include <engine/base.h>

#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

#include <typeindex>

using json = nlohmann::json;

namespace Parsing {
    static sf::Vector2f
    parseVector(json &dict, const std::string &key)
    {
        auto v = dict.find("position")->get<std::vector<float>>();
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
    static Component *
    deserialize(json &dict)
    {
        return new Player;
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
    storage->registerComponent<Transform>("Transform");
    storage->registerComponent<Sprite>("Sprite");
    storage->registerComponent<Camera>("Camera");
    storage->registerComponent<Player>("Player");

    storage->registerSystem(render, {TYPE(Transform), TYPE(Sprite)});
    storage->registerSystem(movePlayer, {TYPE(Transform), TYPE(Player)});
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

    for (auto components : *entities)
    {
        loadEntity(components, state, storage);
    }
}