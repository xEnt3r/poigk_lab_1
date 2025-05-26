#include <vector>
#include <algorithm>
#include <functional> 
#include <memory>
#include <cstdlib>
#include <cmath>
#include <ctime>

#include <raylib.h>
#include <raymath.h>

// --- UTILS ---
namespace Utils {
    inline static float RandomFloat(float min, float max) {
        return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
    }
}

// --- TRANSFORM, PHYSICS, LIFETIME, RENDERABLE ---
struct TransformA {
    Vector2 position{};
    float rotation{};
};

struct Physics {
    Vector2 velocity{};
    float rotationSpeed{};
};

struct Renderable {
    enum Size { SMALL = 1, MEDIUM = 2, LARGE = 4 } size = SMALL;
};

// --- RENDERER ---
class Renderer {
public:
    static Renderer& Instance() {
        static Renderer inst;
        return inst;
    }

    void Init(int w, int h, const char* title) {
        InitWindow(w, h, title);
        SetTargetFPS(60);
        screenW = w;
        screenH = h;
    }

    void Begin() {
        BeginDrawing();
        ClearBackground(BLACK);
    }

    void End() {
        EndDrawing();
    }

    void DrawPoly(const Vector2& pos, int sides, float radius, float rot) {
        DrawPolyLines(pos, sides, radius, rot, WHITE);
    }

    int Width() const {
        return screenW;
    }

    int Height() const {
        return screenH;
    }

private:
    Renderer() = default;

    int screenW{};
    int screenH{};
};

// --- ASTEROID HIERARCHY ---

class Asteroid {
public:
    Asteroid(int screenW, int screenH) {
        init(screenW, screenH);
    }
    void SetupHP() {
        hp = baseHP * static_cast<int>(render.size);
        maxHP = hp;
    }
    virtual ~Asteroid() = default;

    bool Update(float dt) {
        transform.position = Vector2Add(transform.position, Vector2Scale(physics.velocity, dt));
        transform.rotation += physics.rotationSpeed * dt;
        if (transform.position.x < -GetRadius() || transform.position.x > Renderer::Instance().Width() + GetRadius() ||
            transform.position.y < -GetRadius() || transform.position.y > Renderer::Instance().Height() + GetRadius())
            return false;
        return true;
    }
    virtual void Draw() const {
        float barWidth = GetRadius() * 2;
        float hpPercent = (float)hp / (float)maxHP;
        Rectangle backBar = { transform.position.x - barWidth / 2, transform.position.y - GetRadius() - 10, barWidth, 5 };
        Rectangle hpBar = { transform.position.x - barWidth / 2, transform.position.y - GetRadius() - 10, barWidth * hpPercent, 5 };
        DrawRectangleRec(backBar, RED);
        DrawRectangleRec(hpBar, BLUE);
    }

    Vector2 GetPosition() const {
        return transform.position;
    }

    float constexpr GetRadius() const {
        return 16.f * (float)render.size;
    }

    int GetDamage() const {
        return baseDamage * static_cast<int>(render.size);
    }

    int GetSize() const {
        return static_cast<int>(render.size);
    }

    void TakeDamage(int dmg) {
        hp -= dmg;
    }

    bool IsDestroyed() const {
        return hp <= 0;
    }

protected:
    void init(int screenW, int screenH) {
        render.size = static_cast<Renderable::Size>(1 << GetRandomValue(0, 2));

        switch (GetRandomValue(0, 3)) {
        case 0:
            transform.position = { Utils::RandomFloat(0, screenW), -GetRadius() };
            break;
        case 1:
            transform.position = { screenW + GetRadius(), Utils::RandomFloat(0, screenH) };
            break;
        case 2:
            transform.position = { Utils::RandomFloat(0, screenW), screenH + GetRadius() };
            break;
        default:
            transform.position = { -GetRadius(), Utils::RandomFloat(0, screenH) };
            break;
        }

        float maxOff = fminf(screenW, screenH) * 0.1f;
        float ang = Utils::RandomFloat(0, 2 * PI);
        float rad = Utils::RandomFloat(0, maxOff);
        Vector2 center = {
            screenW * 0.5f + cosf(ang) * rad,
            screenH * 0.5f + sinf(ang) * rad
        };

        Vector2 dir = Vector2Normalize(Vector2Subtract(center, transform.position));
        physics.velocity = Vector2Scale(dir, Utils::RandomFloat(SPEED_MIN, SPEED_MAX));
        physics.rotationSpeed = Utils::RandomFloat(ROT_MIN, ROT_MAX);
        transform.rotation = Utils::RandomFloat(0, 360);
    }

    TransformA transform;
    Physics    physics;
    Renderable render;

    int baseDamage = 0;
    int baseHP = 10;
    int hp = 0;
    int maxHP = 0;

    static constexpr float SPEED_MIN = 20.f;
    static constexpr float SPEED_MAX = 120.f;
    static constexpr float ROT_MIN = 40.f;
    static constexpr float ROT_MAX = 150.f;
};

class TriangleAsteroid : public Asteroid {
public:
    TriangleAsteroid(int w, int h) : Asteroid(w, h) {
        baseDamage = 5;
        baseHP = 30;
    }
    void Draw() const override {
        Asteroid::Draw();
        Renderer::Instance().DrawPoly(transform.position, 3, GetRadius(), transform.rotation);
    }
};

class SquareAsteroid : public Asteroid {
public:
    SquareAsteroid(int w, int h) : Asteroid(w, h) {
        baseDamage = 10;
        baseHP = 60;
    }
    void Draw() const override {
        Asteroid::Draw();
        Renderer::Instance().DrawPoly(transform.position, 4, GetRadius(), transform.rotation);
    }
};

class PentagonAsteroid : public Asteroid {
public:
    PentagonAsteroid(int w, int h) : Asteroid(w, h) {
        baseDamage = 15;
        baseHP = 90;
    }
    void Draw() const override {
        Asteroid::Draw();
        Renderer::Instance().DrawPoly(transform.position, 5, GetRadius(), transform.rotation);
    }
};

class RedHeavyAsteroid : public Asteroid {
public:
    RedHeavyAsteroid(int w, int h) : Asteroid(w, h) {
        baseDamage = 20;
        baseHP = 150;
        SetupHP();
    }
    void Draw() const override {
        Asteroid::Draw();
        Renderer::Instance().DrawPoly(transform.position, 6, GetRadius(), transform.rotation);
    }
};

// Shape selector
enum class AsteroidShape { TRIANGLE = 3, SQUARE = 4, PENTAGON = 5, REDHEAVY = 6, RANDOM = 0 };

// Factory
static inline std::unique_ptr<Asteroid> MakeAsteroid(int w, int h, AsteroidShape shape) {
    switch (shape) {
    case AsteroidShape::TRIANGLE:
        return std::make_unique<TriangleAsteroid>(w, h);
    case AsteroidShape::SQUARE:
        return std::make_unique<SquareAsteroid>(w, h);
    case AsteroidShape::PENTAGON:
        return std::make_unique<PentagonAsteroid>(w, h);
    case AsteroidShape::REDHEAVY:
        return std::make_unique<RedHeavyAsteroid>(w, h);
    default: {
        // Randomly select from available shapes 3,4,5,6
        int shapeVal = 3 + GetRandomValue(0, 3); // 3..6
        return MakeAsteroid(w, h, static_cast<AsteroidShape>(shapeVal));
    }
    }
}

// --- PROJECTILE HIERARCHY ---
enum class WeaponType { LASER, BULLET, SIDE_BLASTER, COUNT };


class Projectile {
public:
    Projectile(Vector2 pos, Vector2 vel, int dmg, WeaponType wt)
    {
        transform.position = pos;
        physics.velocity = vel;
        baseDamage = dmg;
        type = wt;
    }
    bool Update(float dt) {
        transform.position = Vector2Add(transform.position, Vector2Scale(physics.velocity, dt));

        if (transform.position.x < 0 ||
            transform.position.x > Renderer::Instance().Width() ||
            transform.position.y < 0 ||
            transform.position.y > Renderer::Instance().Height())
        {
            return true;
        }
        return false;
    }
    void Draw() const {
        if (type == WeaponType::BULLET) {
            DrawCircleV(transform.position, 5.f, WHITE);
        }
        else if (type == WeaponType::LASER) {
            static constexpr float LASER_LENGTH = 30.f;
            Rectangle lr = { transform.position.x - 2.f, transform.position.y - LASER_LENGTH, 4.f, LASER_LENGTH };
            DrawRectangleRec(lr, BLUE);
        }
        else if (type == WeaponType::SIDE_BLASTER) {
            const float radius = 10.f;
            Vector2 p1 = { transform.position.x, transform.position.y - radius };
            Vector2 p2 = { transform.position.x - radius * 0.866f, transform.position.y + radius * 0.5f };
            Vector2 p3 = { transform.position.x + radius * 0.866f, transform.position.y + radius * 0.5f };
            DrawTriangle(p1, p2, p3, GREEN);
        }
    }
    Vector2 GetPosition() const {
        return transform.position;
    }

    float GetRadius() const {
        return (type == WeaponType::BULLET) ? 5.f : 2.f;
    }

    int GetDamage() const {
        return baseDamage;
    }

private:
    TransformA transform;
    Physics    physics;
    int        baseDamage;
    WeaponType type;
};

inline static std::vector<Projectile> MakeProjectile(WeaponType wt, const Vector2 pos, float speed) {
    std::vector<Projectile> result;

    if (wt == WeaponType::LASER) {
        result.push_back(Projectile(pos, { 0, -speed }, 10, wt));
    }
    else if (wt == WeaponType::BULLET) {
        result.push_back(Projectile(pos, { 0, -speed }, 3, wt));
    }
    else if (wt == WeaponType::SIDE_BLASTER) {
        result.push_back(Projectile(pos, { -speed, 0 }, 5, wt));
        result.push_back(Projectile(pos, { speed, 0 }, 5, wt));
    }

    return result;
}


// --- SHIP HIERARCHY ---
class Ship {
public:
    Ship(int screenW, int screenH) {
        transform.position = {
            screenW * 0.5f,
            screenH * 0.5f
        };
        hp = 100;
        maxHP = 100;
        speed = 500.f;
        alive = true;

        // per-weapon fire rate & spacing
        fireRateLaser = 10.f; // shots/sec
        fireRateBullet = 30.f;
        spacingLaser = 40.f; // px between lasers
        spacingBullet = 20.f;
    }
    virtual ~Ship() = default;
    virtual void Update(float dt) = 0;
    virtual void Draw() const = 0;

    void TakeDamage(int dmg) {
        if (!alive) return;
        hp -= dmg;
        if (hp <= 0) alive = false;
    }

    bool IsAlive() const {
        return alive;
    }

    Vector2 GetPosition() const {
        return transform.position;
    }

    virtual float GetRadius() const = 0;

    int GetHP() const {
        return hp;
    }

    int GetMaxHP() const {
        return maxHP;
    }

    float GetFireRate(WeaponType wt) const {
        return (wt == WeaponType::LASER) ? fireRateLaser : fireRateBullet;
    }

    float GetSpacing(WeaponType wt) const {
        return (wt == WeaponType::LASER) ? spacingLaser : spacingBullet;
    }

protected:
    TransformA transform;
    int        hp;
    int        maxHP;
    float      speed;
    bool       alive;
    float      fireRateLaser;
    float      fireRateBullet;
    float      spacingLaser;
    float      spacingBullet;
};

class PlayerShip :public Ship {
public:
    PlayerShip(int w, int h) : Ship(w, h) {
        texture = LoadTexture("spaceship.png");
        GenTextureMipmaps(&texture);
        SetTextureFilter(texture, 2);
        scale = 0.25f;
    }
    ~PlayerShip() {
        UnloadTexture(texture);
    }

    void Update(float dt) override {
        if (alive) {
            if (IsKeyDown(KEY_W)) transform.position.y -= speed * dt;
            if (IsKeyDown(KEY_S)) transform.position.y += speed * dt;
            if (IsKeyDown(KEY_A)) transform.position.x -= speed * dt;
            if (IsKeyDown(KEY_D)) transform.position.x += speed * dt;
        }
        else {
            transform.position.y += speed * dt;
        }
    }

    void Draw() const override {
        if (!alive && fmodf(GetTime(), 0.4f) > 0.2f) return;
        Vector2 dstPos = {
            transform.position.x - (texture.width * scale) * 0.5f,
            transform.position.y - (texture.height * scale) * 0.5f
        };
        DrawTextureEx(texture, dstPos, 0.0f, scale, WHITE);


        float barWidth = 200.f;
        float hpPercent = (float)GetHP() / (float)GetMaxHP();
        Rectangle backBar = { 10, 10, barWidth, 20 };
        Rectangle hpBar = { 10, 10, barWidth * hpPercent, 20 };
        DrawRectangleRec(backBar, RED);
        DrawRectangleRec(hpBar, BLUE);
        DrawText(TextFormat("%d/%d", GetHP(), GetMaxHP()), 20, 10, 20, BLACK);
    }

    float GetRadius() const override {
        return (texture.width * scale) * 0.5f;
    }

private:
    Texture2D texture;
    float     scale;
};

// --- APPLICATION ---
class Application {
public:
    static Application& Instance() {
        static Application inst;
        return inst;
    }

    void Run() {
        srand(static_cast<unsigned>(time(nullptr)));
        Renderer::Instance().Init(C_WIDTH, C_HEIGHT, "Asteroids OOP");

        // Load background texture
        Texture2D background = LoadTexture("background.jpg");
        if (background.id == 0) {
            TraceLog(LOG_WARNING, "Failed to load background.jpg, using black background");
        }

        auto player = std::make_unique<PlayerShip>(C_WIDTH, C_HEIGHT);

        float spawnTimer = 0.f;
        float spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
        WeaponType currentWeapon = WeaponType::LASER;
        float shotTimer = 0.f;
        int points = 0; // Added points counter

        while (!WindowShouldClose()) {
            float dt = GetFrameTime();
            spawnTimer += dt;

            // Update player
            player->Update(dt);

            // Restart logic
            if (!player->IsAlive() && IsKeyPressed(KEY_R)) {
                player = std::make_unique<PlayerShip>(C_WIDTH, C_HEIGHT);
                asteroids.clear();
                projectiles.clear();
                spawnTimer = 0.f;
                spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
                points = 0; // Reset points on restart
            }
            // Asteroid shape switch
            if (IsKeyPressed(KEY_ONE)) {
                currentShape = AsteroidShape::TRIANGLE;
            }
            if (IsKeyPressed(KEY_TWO)) {
                currentShape = AsteroidShape::SQUARE;
            }
            if (IsKeyPressed(KEY_THREE)) {
                currentShape = AsteroidShape::PENTAGON;
            }
            if (IsKeyPressed(KEY_FIVE)) {
                currentShape = AsteroidShape::REDHEAVY;
            }
            if (IsKeyPressed(KEY_FOUR)) {
                currentShape = AsteroidShape::RANDOM;
            }

            // Weapon switch
            if (IsKeyPressed(KEY_TAB)) {
                currentWeapon = static_cast<WeaponType>((static_cast<int>(currentWeapon) + 1) % static_cast<int>(WeaponType::COUNT));
            }

            // Shooting
            {
                if (player->IsAlive() && IsKeyDown(KEY_SPACE)) {
                    shotTimer += dt;
                    float interval = 1.f / player->GetFireRate(currentWeapon);
                    float projSpeed = player->GetSpacing(currentWeapon) * player->GetFireRate(currentWeapon);

                    while (shotTimer >= interval) {
                        Vector2 p = player->GetPosition();
                        p.y -= player->GetRadius();
                        auto shots = MakeProjectile(currentWeapon, p, projSpeed);
                        projectiles.insert(projectiles.end(), shots.begin(), shots.end());
                        shotTimer -= interval;
                    }
                }
                else {
                    float maxInterval = 1.f / player->GetFireRate(currentWeapon);

                    if (shotTimer > maxInterval) {
                        shotTimer = fmodf(shotTimer, maxInterval);
                    }
                }
            }

            // Spawn asteroids
            if (spawnTimer >= spawnInterval && asteroids.size() < MAX_AST) {
                asteroids.push_back(MakeAsteroid(C_WIDTH, C_HEIGHT, currentShape));
                spawnTimer = 0.f;
                spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
            }

            // Update projectiles - check if in boundries and move them forward
            {
                auto projectile_to_remove = std::remove_if(projectiles.begin(), projectiles.end(),
                    [dt](auto& projectile) {
                        return projectile.Update(dt);
                    });
                projectiles.erase(projectile_to_remove, projectiles.end());
            }

            // Projectile-Asteroid collisions O(n^2)
            for (auto pit = projectiles.begin(); pit != projectiles.end();) {
                bool removed = false;

                for (auto ait = asteroids.begin(); ait != asteroids.end(); ++ait) {
                    float dist = Vector2Distance(pit->GetPosition(), (*ait)->GetPosition());
                    if (dist < pit->GetRadius() + (*ait)->GetRadius()) {
                        (*ait)->TakeDamage(pit->GetDamage());

                        pit = projectiles.erase(pit);
                        removed = true;

                        if ((*ait)->IsDestroyed()) {
                            ait = asteroids.erase(ait);
                            points++; // Add point when asteroid is destroyed
                        }
                        break;
                    }
                }
                if (!removed) {
                    ++pit;
                }
            }

            // Asteroid-Ship collisions
            {
                auto remove_collision =
                    [&player, dt](auto& asteroid_ptr_like) -> bool {
                    if (player->IsAlive()) {
                        float dist = Vector2Distance(player->GetPosition(), asteroid_ptr_like->GetPosition());

                        if (dist < player->GetRadius() + asteroid_ptr_like->GetRadius()) {
                            player->TakeDamage(asteroid_ptr_like->GetDamage());
                            return true; // Mark asteroid for removal due to collision
                        }
                    }
                    if (!asteroid_ptr_like->Update(dt)) {
                        return true;
                    }
                    return false; // Keep the asteroid
                    };
                auto asteroid_to_remove = std::remove_if(asteroids.begin(), asteroids.end(), remove_collision);
                asteroids.erase(asteroid_to_remove, asteroids.end());
            }

            // Render everything
            {
                Renderer::Instance().Begin();

                // Draw background if loaded
                if (background.id != 0) {
                    DrawTexture(background, 0, 0, WHITE);
                }

                const char* weaponName = nullptr;
                switch (currentWeapon) {
                case WeaponType::LASER: weaponName = "LASER"; break;
                case WeaponType::BULLET: weaponName = "BULLET"; break;
                case WeaponType::SIDE_BLASTER: weaponName = "SIDE_BLASTER"; break;
                }

                DrawText(TextFormat("Weapon: %s", weaponName), 10, 40, 20, BLUE);
                DrawText(TextFormat("Points: %d", points), 10, 70, 20, GREEN); // Display points

                for (const auto& projPtr : projectiles) {
                    projPtr.Draw();
                }
                for (const auto& astPtr : asteroids) {
                    astPtr->Draw();
                }

                player->Draw();

                Renderer::Instance().End();
            }
        }

        // Unload background texture if it was loaded
        if (background.id != 0) {
            UnloadTexture(background);
        }
    }

private:
    Application()
    {
        asteroids.reserve(1000);
        projectiles.reserve(10'000);
    };

    std::vector<std::unique_ptr<Asteroid>> asteroids;
    std::vector<Projectile> projectiles;

    AsteroidShape currentShape = AsteroidShape::TRIANGLE;

    static constexpr int C_WIDTH = 1600;
    static constexpr int C_HEIGHT = 1600;
    static constexpr size_t MAX_AST = 150;
    static constexpr float C_SPAWN_MIN = 0.5f;
    static constexpr float C_SPAWN_MAX = 3.0f;

    static constexpr int C_MAX_ASTEROIDS = 1000;
    static constexpr int C_MAX_PROJECTILES = 10'000;
};

int main() {
    Application::Instance().Run();
    return 0;
}