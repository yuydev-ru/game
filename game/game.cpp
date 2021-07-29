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


struct  Enemy : Component
{

    static Component *
    deserialize(Parser &parser)
    {
        return new Enemy;
    }
};

struct Health : Component
{
    int health = 100;

    static Component *
    deserialize(Parser &parser)
    {
        return new Health;
    }
};

struct Weapon: Component
{

    int damage = 1;
    float cooldown = 5;

    static Component *
    deserialize(Parser &parser)
    {
        return new Weapon;
    }
};


void
combatSystem(GameState *state, Storage *storage, const Entity id)
{
    auto a = storage->getComponent<Health>(id);
    auto b = storage->getComponent<Transform>(id);
    auto c = storage->getComponent<Weapon>(id);
    float radius = 30000;

    c->cooldown -= state->deltaTime;

    if (state->axes["attack"] == true )
    {
        if (c->cooldown > 0)
        {
           // std::cout<<"your cd:"<<c->cooldown<<"\n";
            return;
        }
        else
        {

            for(auto now_id: storage->usedIds)
            {
                auto now_typ = storage->getComponent<Enemy>(now_id);

                if (now_typ!=NULL)
                {
                    auto now_tran = storage->getComponent<Transform>(now_id);
                    float dist = pow((b->position.x-now_tran->position.x),2) + pow((b->position.y-now_tran->position.y),2);

                    if (dist < radius){
                        auto now_hp_en = storage->getComponent<Health>(now_id);

                        now_hp_en->health -= c -> damage;
                        c->cooldown = 5;

                        std::cout << "attack" << " " << now_hp_en->health << "\n";
                    }
                }
            }
        }
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
initializeEngine(GameState *state, Storage *storage)
{
    storage->registerComponent<Player>("Player");
    storage->registerComponent<Enemy>("Enemy");
    storage->registerComponent<Health>("Health");
    storage->registerComponent<Weapon>("Weapon");

    storage->registerSystem(combatSystem, {TYPE(Player), TYPE(Health), TYPE(Weapon), TYPE(Transform)});
    storage->registerSystem(movePlayer, {TYPE(Transform), TYPE(Player), TYPE(Collider), TYPE(Physics)});
}