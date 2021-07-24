#include <SFML/Audio.hpp>
#include <engine/interface.h>
#include <engine/base.h>
#include <engine/parsing.h>
#include <engine/components.h>

#include <set>
#include <typeindex>
#include <cmath>

/* Components */

struct Player : Component
{
    float speed = 200;

    static Component *
    deserialize(Parsing::configFile &dict)
    {
        return new Player;
    }
};

struct Sound : Component
{
    // TODO (vincento): Добавить вектор звуков (для их удаления после воспроизв-я). Лимит в SFML - 256.

    std::string name;
    std::string assetPath;
    float volume;
    bool isLooped;
    bool playOnStart = false;

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
        s->playOnStart = Parsing::parseElement<bool>(dict, "playOnStart");

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
pushOut(GameState *state, Storage *storage, const Entity id)
{
    auto coll = storage->getComponent<Collider>(id);
    auto p = storage->getComponent<Physics>(id);
    if (coll != nullptr && !coll->collisionList.empty() && !coll->allowCollision)
    {

        auto t = storage->getComponent<Transform>(id);
        sf::Vector2f move = coll->normal * coll->penetration;
        move.x *= p->activeAxes.x;
        move.y *= p->activeAxes.y;
        t->position += move;
    }
}
void
movePlayer(GameState *state, Storage *storage, const Entity id)
{
    auto t = storage->getComponent<Transform>(id);
    auto c = storage->getComponent<Collider>(id);
    auto p = storage->getComponent<Physics>(id);
    auto s = storage->getComponent<Sound>(id);
    sf::Vector2f move = {state->axes["horizontal"], state->axes["vertical"]};
    if (state->axes["jump"] == 1 && c->normal.y == 1)
    {
        p->speed.y += 400.f;

        s->sound.play();
    }
    if (c->normal.y != 1)
    {
        move.y = 0;
    }

    // Нормализуем верктор move
    float length = std::sqrt((move.x * move.x) + (move.y * move.y));
    if (length != 0)
    {
        move /= length;
    }

    move.x *= storage->getComponent<Player>(id)->speed;
    move.y *= storage->getComponent<Player>(id)->speed;

    t->position += move * state->deltaTime;
}

void
setupSound(GameState *state, Storage *storage, const Entity id)
{
    //TODO: Эта функция должна вызываться 1 раз вместо постоянного вызова в game loop.
    auto snd = storage->getComponent<Sound>(id);
    if (snd->playOnStart == true)
    {
        snd->play();
        snd->playOnStart = false;
    }
}
/* ******* */

// NOTE(guschin): Возможно, эту функцию можно генерировать автоматически.
void
initializeEngine(GameState *state, Storage *storage)
{
    storage->registerComponent<Player>("Player");
    storage->registerComponent<Sound>("Sound");

    storage->registerSystem(render, {TYPE(Transform), TYPE(Sprite)});
    storage->registerSystem(movePlayer, {TYPE(Transform), TYPE(Player), TYPE(Collider), TYPE(Physics)});
    storage->registerSystem(setupSound,{TYPE(Sound)});
}
