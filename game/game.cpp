#include <engine/interface.h>
#include <engine/base.h>
#include <engine/parsing.h>

#include <set>
#include <iostream>
#include <string>
#include <typeindex>
#include <cmath>

/* Components */

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
    float speed = 200;

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
    // NOTE(Roma): Разница между центром объекта и центром его коллайдера.
    sf::Vector2f deltaCenter = {0, 0};
    // NOTE(Roma): Список всех объектов, с которыми пересекается данный.
    std::set<Entity> collisionList;
    bool allowCollision = false;
    // NOTE(Roma): Глубина проникновения одного коллайдера в другой.
    float penetration = 0;
    // NOTE(Roma): Направление коллизии.
    sf::Vector2f normal = {0, 0};

    static Component *
    deserialize(Parsing::configFile &dict)
    {
        auto c = new Collider;

        c->deltaCenter = Parsing::parseVector2<float>(dict, "deltaCenter");
        c->allowCollision = Parsing::parseElement<bool>(dict, "allowCollision");
        c->width = Parsing::parseElement<float>(dict, "width");
        c->height = Parsing::parseElement<float>(dict, "height");

        return c;
    }
};
struct Physics : Component
{
    sf::Vector2f speed = {0, 0};
    sf::Vector2f position = {0, 0};
    sf::Vector2f activeAxes = {1, 1};
    const float gravityAcceleration = 500;
    float mass = 1;

    bool allowGravity = true;

    std::map<std::string, sf::Vector2f> forces = {{"gravity", {0, 0}}
                                                  , {"normal", {0, 0}}
                                                  , {"friction", {0, 0}}};
    sf::Vector2f resForce = {0, 0};
    void
    evalResForce()
    {
        resForce = {0, 0};
        for (const auto& force : forces)
        {
            resForce += force.second;
        }
    }

    static Component *
    deserialize(Parsing::configFile &dict)
    {
        auto p = new Physics;

        p->mass = Parsing::parseElement<float>(dict, "mass");
        p->allowGravity = Parsing::parseElement<bool>(dict, "allowGravity");
        p->activeAxes = Parsing::parseVector2<float>(dict, "activeAxes");

        return p;
    }
};
/* Systems */

void
physics(GameState *state, Storage *storage, const Entity id)
{
    auto p = storage->getComponent<Physics>(id);
    auto coll = storage->getComponent<Collider>(id);

    if (p->allowGravity && coll->normal.y != 1)
    {
        p->forces["gravity"] = {0, -1 * p->gravityAcceleration * p->mass};
        p->forces["normal"] = {0, 0};
    }

    if (p->allowGravity && coll->normal.y == 1)
    {
        p->speed.y = 0;
        p->forces["normal"] = {0, -1 * p->forces["gravity"].y};
    }
    if (coll->normal.y == -1)
    {
        p->speed.y = 0;
    }
    p->evalResForce();

    auto resForce = p->resForce / p->mass;

    p->speed += resForce * state->deltaTime;
    auto t = storage->getComponent<Transform>(id);

    if (p->activeAxes.x == 1)
    {
        t->position.x += p->speed.x * state->deltaTime;
    }

    if (p->activeAxes.y == 1)
    {
        t->position.y += p->speed.y * state->deltaTime;
    }

}

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

    sf::Vector2f move = {state->axes["horizontal"], state->axes["vertical"]};
    if (state->axes["jump"] == 1 && c->normal.y == 1)
    {
        p->speed.y += 400.f;
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
collision (GameState *state, Storage *storage, const Entity id)
{
    auto c = storage->getComponent<Collider>(id);
    auto t = storage->getComponent<Transform>(id);
    auto tPos = t->position;
    auto dC = c->deltaCenter;
    float w = c->width * 0.5f;
    float h = c->height * 0.5f;

    for (Entity id2 : storage->usedIds)
    {
        // NOTE(Roma): Берётся коллайдер другого объекта, и если он есть и этот объект не совпадает с текущим.
        auto c2 = storage->getComponent<Collider>(id2);
        if (c2 != nullptr && id2 != id)
        {
            auto t2 = storage->getComponent<Transform>(id2);
            auto tPos2 = t2->position;
            auto dC2 = c2->deltaCenter;
            float w2 = c2->width * 0.5f;
            float h2 = c2->height * 0.5f;

            sf::Vector2f betweenCenters = {tPos.x + dC.x - tPos2.x - dC2.x, tPos.y + dC.y - tPos2.y - dC2.y};
            // Степень наложения одного коллайдера на другой по оси Х.
            float overlapX = w + w2 - (float) fabs(betweenCenters.x);

            if (overlapX > 0)
            {
                // Степень наложения по оси Y.
                float overlapY = h + h2 - (float) fabs(betweenCenters.y);

                if (overlapY > 0)
                {
                    // Выталкивать нужно в ту сторону, где степень наложения меньше.
                    if (overlapX < overlapY)
                    {
                        if (betweenCenters.x > 0)
                        {
                            c->normal = {1, 0 };
                            c2->normal = {-1, 0 };
                        }
                        else
                        {
                            c->normal = {-1, 0};
                            c2->normal = {1, 0};
                        }
                        c->penetration = overlapX;
                        c2->penetration = overlapX;
                        c->collisionList.insert(id2);
                        c2->collisionList.insert(id);
                        break;
                    }
                    else
                    {
                        if (betweenCenters.y > 0)
                        {
                            c->normal = {0, 1 };
                            c2->normal = {0, -1 };
                        }
                        else
                        {
                            c->normal = {0, -1};
                            c2->normal = {0, 1 };
                        }
                        c->penetration = overlapY;
                        c2->penetration = overlapY;
                        c->collisionList.insert(id2);
                        c2->collisionList.insert(id);
                        break;
                    }
                }
            }
            c->collisionList.erase(id2);
            c2->collisionList.erase(id);
            c->normal = {0, 0};
            c->penetration = 0;
            c2->normal = {0, 0};
            c2->penetration = 0;
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
    storage->registerComponent<Physics>("Physics");

    storage->registerSystem(render, {TYPE(Transform), TYPE(Sprite)});
    storage->registerSystem(movePlayer, {TYPE(Transform), TYPE(Player), TYPE(Collider), TYPE(Physics)});
    storage->registerSystem(collision, {TYPE(Collider), TYPE(Player)});
    storage->registerSystem(pushOut, {TYPE(Collider), TYPE(Physics), TYPE(Transform)});
    storage->registerSystem(physics, {TYPE(Collider), TYPE(Physics), TYPE(Transform)});

}
