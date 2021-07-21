#include <SFML/Audio.hpp>
#include <engine/interface.h>
#include <engine/base.h>

#include <set>
#include <iostream>
#include <string>
#include <typeindex>
#include <cmath>

struct Transform : Component
{
    sf::Vector2f position = {0, 0};
    sf::Vector2f scale = {1, 1};

    static Component *
    deserialize(Parsing::configFile &dict)
    {
        auto t = new Transform;

        t->position = Parsing::parseVector2<float>(dict, "position");
        t->scale = Parsing::parseVector2<float>(dict, "scale");

        return t;
    }
};

struct Sprite : Component
{
    std::string assetPath;
    sf::Image image;
    sf::Texture texture;
    sf::Sprite sprite;

    static Component *
    deserialize(Parsing::configFile &dict)
    {
        auto spr = new Sprite;

        spr->assetPath = Parsing::parseElement<std::string>(dict, "assetPath");

        spr->image.loadFromFile(spr->assetPath);
        spr->texture.loadFromImage(spr->image);
        spr->sprite.setTexture(spr->texture);
        sf::IntRect rect = { 0, 0
                           , (int) spr->image.getSize().x
                           , (int) spr->image.getSize().y };
        spr->sprite.setTextureRect(rect);

        return spr;
    }

};
struct Camera : Component
{
    sf::Vector2f scale = {1, 1};

    static Component *
    deserialize(Parsing::configFile &dict)
    {
        auto c = new Camera;

        c->scale = Parsing::parseVector2<float>(dict, "scale");

        return c;
    }
};
struct Player : Component
{
    float speed = .5;

    static Component *
    deserialize(Parsing::configFile &dict)
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
    deserialize(Parsing::configFile &dict)
    {
        auto c = new Collider;

        c->deltaCenter = Parsing::parseVector2<float>(dict, "deltaCenter");
        c->width = Parsing::parseElement<float>(dict, "width");
        c->height = Parsing::parseElement<float>(dict, "height");

        return c;
    }
};

struct Sound : Component
{
    // TODO (vincento): Добавить вектор звуков (для их удаления после воспроизв-я). Лимит в SFML - 256.

    std::string name;
    std::string assetPath;
    float volume;
    bool isLooped;
    sf::SoundBuffer buffer;
    sf::Sound sound;
    bool isLoaded = false;

    static Component *
    deserialize(Parsing::configFile &dict)
    {
        auto s = new Sound;
        s->name = Parsing::parseElement<std::string>(dict, "name");
        s->assetPath = Parsing::parseElement<std::string>(dict, "assetPath");
        s->volume =  Parsing::parseElement<float>(dict, "volume");
        s->isLooped = Parsing::parseElement<bool>(dict, "isLooped");
        if (s->buffer.loadFromFile(s->assetPath))
        {
            s->isLoaded = true;
            s->sound.setBuffer(s->buffer);
            s->sound.setLoop(s->isLooped);
            s->sound.setVolume(s->volume);
            std::cout <<"Sound registered, assetPath: " << s->assetPath << "\n";
        }
        else
        {
            std::cout << "ERROR: Sound doesn't load!\n";
        }
        return s;
    }
    void
    play()
    {
        if (this->isLoaded)
        {
            this->sound.play();
        }
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
void soundTest(GameState *state, Storage *storage, const Entity id)
{
    if (state->axes["vertical"] > 0.0f)
    {
        auto snd = storage->getComponent<Sound>(id);
        if (snd->isLoaded && snd->name == "music")
        {
            snd->sound.play();
        }
    }
    if (state->axes["interact"] > 0.0f)
    {
        auto snd = storage->getComponent<Sound>(id);
        if (snd->isLoaded && snd->name == "sound")
        {
            snd->sound.play();
        }
    }
}

// NOTE(guschin): Возможно, эту функцию можно генерировать автоматически.
void
initializeEngine(GameState *state, Storage *storage)
{

    storage->registerComponent<Transform>("Transform");
    storage->registerComponent<Sprite>("Sprite");
    storage->registerComponent<Camera>("Camera");
    storage->registerComponent<Player>("Player");
    storage->registerComponent<Collider>("Collider");
    storage->registerComponent<Sound>("Sound");
    storage->registerSystem(render, {TYPE(Transform), TYPE(Sprite)});
    storage->registerSystem(movePlayer, {TYPE(Transform), TYPE(Player)});
    storage->registerSystem(updateCollider, {TYPE(Transform), TYPE(Sprite)});
    storage->registerSystem(collision, {TYPE(Collider), TYPE(Player)});
    storage->registerSystem(soundTest,{TYPE(Sound)});
}