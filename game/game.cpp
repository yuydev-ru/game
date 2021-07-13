#include <engine/interface.h>
#include <engine/base.h>

#include <set>
#include <iostream>
#include <cmath>

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
    float speed = .5;
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
};
struct Physics : Component
{
    float speed = 0;
    // NOTE(Roma): Вектор направления движения.
    sf::Vector2f dir = {0, 0};
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

// TODO(Roma): Нужно исправить коллизию при перемещении по диагонали
void
pushOut(GameState *state, Storage *storage, const Entity id)
{
    for (Entity ent_id : storage->usedIds)
    {
        auto coll = storage->getComponent<Collider>(ent_id);
        if (coll != nullptr && !coll->collisionList.empty() && !coll->allowCollision)
        {
            for (auto id2 : coll->collisionList)
            {
                auto coll2 = storage->getComponent<Collider>(id2);
                auto t = storage->getComponent<Transform>(ent_id);
                auto t2 = storage->getComponent<Transform>(id2);
                sf::Vector2f c = {t->position.x + coll->deltaCenter.x, t->position.y + coll->deltaCenter.y};
                sf::Vector2f c2 = {t2->position.x + coll2->deltaCenter.x, t2->position.y + coll2->deltaCenter.y};
                sf::Vector2f dC = {c.x - c2.x, c.y - c2.y};
                auto p = storage->getComponent<Physics>(ent_id);
                if (p->dir.x != 0 && p->dir.y != 0)
                {
                        t->position.x += p->speed * dC.x;
                        t->position.y += p->speed * dC.y;
                }
                else
                {
                    sf::Vector2f pushDir = {p->dir.x * -1, p->dir.y * -1};
                    t->position.x += p->speed * pushDir.x;
                    t->position.y += p->speed * pushDir.y;
                }
            }
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
    // NOTE(Roma): У первого объекта берутся коллайдер и трансформ для обновления координат углов коллайдера
    auto c = storage->getComponent<Collider>(id);
    auto t = storage->getComponent<Transform>(id);
    auto tPos = t->position;
    auto tSc = t->scale;
    auto dc = c->deltaCenter;
    float w = c->width * 0.5f * tSc.x;
    float h = c->height * 0.5f * tSc.y;
    // NOTE(Roma): К центру объекта прибавляется разница между ним и центром коллайдера, чтобы вычислить координаты
    // последнего. Затем прибавляется или убавляется половина длины и ширины коллайдера с учётом масштаба,
    // чтобы вычислить левый нижний или правый верхний уголы.
    c->leftDownCorner = {tPos.x + dc.x - w, tPos.y + dc.y - h};
    c->rightUpCorner = {tPos.x + dc.x + w, tPos.y + dc.y + h};
    auto lC = c->leftDownCorner;
    auto rC = c->rightUpCorner;

    for (Entity id2 : storage->usedIds)
    {
        // NOTE(Roma): Берётся коллайдер другого объекта, и если он есть и этот объект не совпадает с текущим,
        auto c2 = storage->getComponent<Collider>(id2);
        if (c2 != nullptr && id2 != id)
        {
            // то обновляем координаты углов его коллайдера
            auto t2 = storage->getComponent<Transform>(id2);
            auto tPos2 = t2->position;
            auto tSc2 = t2->scale;
            auto dc2 = c2->deltaCenter;
            float w2 = c2->width * 0.5f * tSc2.x;
            float h2 = c2->height * 0.5f * tSc2.y;
            c2->leftDownCorner = {tPos2.x + dc2.x - w2, tPos2.y + dc2.y - h2};
            c2->rightUpCorner = {tPos2.x + dc2.x + w2, tPos2.y + dc2.y + h2};
            auto lC2 = c2->leftDownCorner;
            auto rC2 = c2->rightUpCorner;
            // NOTE(Roma): и проверяем по углам, не пересекаются ли они.
            if (rC.x < lC2.x || lC.x > rC2.x || rC.y < lC2.y || lC.y > rC2.y)
            {
                // В случае, когда не пересекаются, нужно удалить в списке пересечений первого объекта
                // второй объект. Аналогично со списком второго.
                c->collisionList.erase(id2);
                c2->collisionList.erase(id);
                continue;
            }
            // Иначе, поскольку они пересекаются, добавляем объекты в списки друг друга.
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
    storage->registerComponent<Physics>();

    storage->registerSystem(render, {TYPE(Transform), TYPE(Sprite)});
    storage->registerSystem(movePlayer, {TYPE(Transform), TYPE(Player)});
    storage->registerSystem(collision, {TYPE(Collider), TYPE(Player)});
    storage->registerSystem(pushOut, {TYPE(Collider), TYPE(Physics), TYPE(Transform)});

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
    p->speed = 0.1f;

    Entity e2 = storage->createEntity();
    auto e2_t = storage->addComponent<Transform>(e2);
    e2_t->scale = {0.55f, 0.1f};
    auto spr2 = storage->addComponent<Sprite>(e2);
    storage->addComponent<Physics>(e2);
    spr2->assetPath = "assets/images/cube.jpg";
    e2_t->position = {0.f, -240.f};

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
    c->width = (float) spr1->image.getSize().x;
    c->height = (float) spr1->image.getSize().y;
    c2->width = (float) spr2->image.getSize().x;
    c2->height = (float) spr2->image.getSize().y;

    Entity camera = storage->createEntity();
    storage->addComponent<Transform>(camera);
    auto cam = storage->addComponent<Camera>(camera);
    cam->scale = {1, 1};
    state->currentCamera = camera;
}