#include <glm/vec2.hpp>

#include <set>
#include <typeindex>
#include <cmath>

#include <engine/interface.h>
#include <engine/base.h>
#include <engine/parser.h>
#include <engine/components.h>

/* Components */

struct Player : herb::Component
{
    float speed = 200;

    static herb::Component *
    deserialize(herb::Parser &parser)
    {
        return new Player;
    }
};

void
movePlayer(herb::GameState *state, herb::Storage *storage, const herb::Entity id)
{
    auto t = storage->getComponent<herb::Transform>(id);
    auto c = storage->getComponent<herb::Collider>(id);
    auto p = storage->getComponent<herb::Physics>(id);
    auto s = storage->getComponent<herb::Sound>(id);

    glm::vec2 move = {state->axes["horizontal"], state->axes["vertical"]};
    if (state->axes["jump"] == 1 && c->normal.y == 1)
    {
        p->speed.y += 400.f;
        s->play();
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

/* ******* */

// NOTE(guschin): Возможно, эту функцию можно генерировать автоматически.
void
initializeEngine(herb::GameState *state, herb::Storage *storage)
{
    storage->registerComponent<Player>("Player");
    storage->registerSystem(movePlayer, {TYPE(herb::Transform), TYPE(Player), TYPE(herb::Collider), TYPE(herb::Physics)});
}
