#include <SFML/Audio.hpp>
#include <engine/interface.h>
#include <engine/base.h>
#include <engine/parser.h>
#include <engine/components.h>

#include <set>
#include <typeindex>
#include <cmath>

/* Components */

struct Player : Component
{
    float speed = 200;

    static Component *
    deserialize(Parser &parser)
    {
        return new Player;
    }
};

void
movePlayer(GameState *state, Storage *storage, const Entity id)
{
    auto t = storage->getComponent<Transform>(id);
    auto c = storage->getComponent<Collider>(id);
    auto p = storage->getComponent<Physics>(id);
    auto s = storage->getComponent<Sound>(id);
    auto anim = storage->getComponent<AnimationController>(id);

    sf::Vector2f move = {state->axes["horizontal"], state->axes["vertical"]};
    if (state->axes["jump"] == 1 && c->normal.y == 1)
    {
        p->speed.y += 600.f;
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

    if (move.x == 0)
    {
        if (anim->currentAnimation->name != "idle")
        {
            anim->setAnimation("idle");
        }
    }

    if (move.x > 0)
    {
        if (anim->currentAnimation->name != "run-right")
        {
            anim->setAnimation("run-right");
        }
    }
    if (move.x < 0)
    {
        if (anim->currentAnimation->name != "run-left")
        {
            anim->setAnimation("run-left");
        }
    }

    t->position += move * state->deltaTime;
}

/* ******* */

// NOTE(guschin): Возможно, эту функцию можно генерировать автоматически.
void
initializeEngine(GameState *state, Storage *storage)
{
    storage->registerComponent<Player>("Player");
    storage->registerSystem(movePlayer, {TYPE(Transform), TYPE(Player), TYPE(Collider), TYPE(Physics), TYPE(AnimationController)});
}