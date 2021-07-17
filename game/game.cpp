#include <engine/interface.h>
#include <engine/base.h>

#include <cmath>
#include <set>
#include <iostream>
#include <map>
#include <string>

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
struct Player : Component
{
    float speed = 200;
};
struct Collider : Component
{
    float width = 0;
    float height = 0;
    // NOTE(Roma): Разница между центром объекта и центром его коллайдера.
    sf::Vector2f deltaCenter = {0, 0};
    // NOTE(Roma): Левый нижний и правый верхний углы, необходимые для обнаружения пересечения двух коллайдеров.
    sf::Vector2f leftDownCorner = {0, 0};
    sf::Vector2f rightUpCorner = {0, 0};
    // NOTE(Roma): Список всех объектов, с которыми пересекается данный.
    std::set<Entity> collisionList;
    // NOTE(Roma): Разрешена ли коллизия: если нет, то объекты не смогут пересекаться.
    bool allowCollision = false;
    // NOTE(Roma): Глубина проникновения одного коллайдера в другой.
    float penetration = 0;
    // NOTE(Roma): Направление коллизии.
    sf::Vector2f normal = {0, 0};
};
struct Physics : Component
{
    float speed = 0;
    sf::Vector2f dirSpeed = {0, 0};
    sf::Vector2f position = {0, 0};
    sf::Vector2f activeAxes = {1, 1};
    const float gravityAcceleration = 0.05;
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
        p->speed = 0;
        p->dirSpeed.y = 0;
        p->forces["normal"] = {0, -1 * p->forces["gravity"].y};
    }
    p->evalResForce();

    auto resForce = p->resForce;

    float acceleration = std::sqrt(resForce.x * resForce.x + resForce.y * resForce.y) / p->mass;
    if (acceleration != 0)
    {
        p->dirSpeed = resForce / acceleration;
    }
    p->speed += acceleration;
    auto t = storage->getComponent<Transform>(id);

    if (p->activeAxes.x == 1)
    {
        t->position.x += p->speed * p->dirSpeed.x * state->deltaTime;
    }

    if (p->activeAxes.y == 1)
    {
        t->position.y += p->speed * p->dirSpeed.y * state->deltaTime;
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

    sf::Vector2f move = {state->axes["horizontal"], state->axes["vertical"]};

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
                        continue;
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
                        continue;
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
    storage->registerComponent<Transform>();
    storage->registerComponent<Sprite>();
    storage->registerComponent<Camera>();
    storage->registerComponent<Player>();
    storage->registerComponent<Collider>();
    storage->registerComponent<Physics>();

    storage->registerSystem(render, {TYPE(Transform), TYPE(Sprite)});
    storage->registerSystem(movePlayer, {TYPE(Transform), TYPE(Player)});
    storage->registerSystem(collision, {TYPE(Collider), TYPE(Player)});
    storage->registerSystem(pushOut, {TYPE(Collider), TYPE(Physics), TYPE(Transform)});
    storage->registerSystem(physics, {TYPE(Collider), TYPE(Physics), TYPE(Transform)});

}

// NOTE(guschin): В этой сцене должна загружаться указанная сцена, но
//  парсинга когфигов пока что нет, поэтому пока что так.
void
loadScene(const Config *config, const std::string& sceneName, GameState *state, Storage *storage)
{
    Entity e1 = storage->createEntity();
    auto e1_t = storage->addComponent<Transform>(e1);
    storage->addComponent<Player>(e1);
    storage->addComponent<Physics>(e1);
    e1_t->scale = {0.1f, 0.1f};
    auto spr1 = storage->addComponent<Sprite>(e1);
    spr1->assetPath = "assets/images/cube.jpg";
    auto p = storage->getComponent<Physics>(e1);
    e1_t->position = {0, 200.f};
    p->mass = 1;

    Entity e2 = storage->createEntity();
    auto e2_t = storage->addComponent<Transform>(e2);
    e2_t->scale = {0.55f, 0.1f};
    auto spr2 = storage->addComponent<Sprite>(e2);
    auto p2 =  storage->addComponent<Physics>(e2);
    spr2->assetPath = "assets/images/cube.jpg";
    e2_t->position = {0.f, -240.f};
    p2->allowGravity = false;

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
    c->width = (float) spr1->image.getSize().x * e1_t->scale.x;
    c->height = (float) spr1->image.getSize().y * e1_t->scale.y;
    c2->width = (float) spr2->image.getSize().x * e2_t->scale.x;
    c2->height = (float) spr2->image.getSize().y * e2_t->scale.y;
    p2->activeAxes = {0, 0};

    Entity camera = storage->createEntity();
    storage->addComponent<Transform>(camera);
    auto cam = storage->addComponent<Camera>(camera);
    cam->scale = {1, 1};
    state->currentCamera = camera;
}