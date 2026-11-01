// Extra Vehicles - Pelota Rebote, Serpiente, portales de velocidad y triggers faciles.
// PRIMER BORRADOR: no esta compilado ni probado. Las partes mas fragiles estan
// marcadas con "PUNTO FRAGIL". Si algo no compila o crashea, manda el error.

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

namespace ev {
    constexpr int ID_BALL  = 7001;
    constexpr int ID_SNAKE = 7002;
    constexpr int ID_S02   = 7003; // x0.2
    constexpr int ID_S04   = 7004; // x0.4
    constexpr int ID_S20   = 7005; // x2
    constexpr int ID_S30   = 7006; // x3
    constexpr int ID_S70   = 7007; // x7
    constexpr int FIRST_ID = 7001;
    constexpr int LAST_ID  = 7007;

    // En GD la velocidad "1x" interna es 0.9
    constexpr float NORMAL_SPEED = 0.9f;

    inline bool isOurs(int id) { return id >= FIRST_ID && id <= LAST_ID; }

    inline std::string frameFor(int id) {
        switch (id) {
            case ID_BALL:  return "ball_portal.png"_spr;
            case ID_SNAKE: return "snake_portal.png"_spr;
            case ID_S02:   return "speed_02.png"_spr;
            case ID_S04:   return "speed_04.png"_spr;
            case ID_S20:   return "speed_20.png"_spr;
            case ID_S30:   return "speed_30.png"_spr;
            default:       return "speed_70.png"_spr;
        }
    }

    inline float speedMultFor(int id) {
        switch (id) {
            case ID_S02: return 0.2f;
            case ID_S04: return 0.4f;
            case ID_S20: return 2.0f;
            case ID_S30: return 3.0f;
            case ID_S70: return 7.0f;
            default:     return 1.0f;
        }
    }

    inline ccColor4F colorFor(int id) {
        switch (id) {
            case ID_BALL:  return ccc4f(1.0f, 0.55f, 0.0f, 1.0f);
            case ID_SNAKE: return ccc4f(0.15f, 0.9f, 0.35f, 1.0f);
            case ID_S02:   return ccc4f(0.5f, 0.35f, 1.0f, 1.0f);
            case ID_S04:   return ccc4f(0.25f, 0.6f, 1.0f, 1.0f);
            case ID_S20:   return ccc4f(1.0f, 0.8f, 0.15f, 1.0f);
            case ID_S30:   return ccc4f(1.0f, 0.45f, 0.1f, 1.0f);
            default:       return ccc4f(1.0f, 0.1f, 0.25f, 1.0f);
        }
    }

    inline const char* soundFor(int id) {
        switch (id) {
            case ID_BALL:
            case ID_SNAKE: return "portal_vehicle.wav";
            case ID_S02:
            case ID_S04:   return "portal_slow.wav";
            case ID_S70:   return "portal_x7.wav";
            default:       return "portal_fast.wav";
        }
    }

    inline void playSfx(char const* file, float volume = 1.0f) {
        if (!Mod::get()->getSettingValue<bool>("play-sounds")) return;
        auto path = fmt::format("{}/{}", Mod::get()->getID(), file);
        FMODAudioEngine::sharedEngine()->playEffect(path, 1.0f, 1.0f, volume);
    }

    inline void burst(CCNode* parent, CCPoint pos, ccColor4F c) {
        if (!parent) return;
        auto p = CCParticleExplosion::create();
        auto texPath = fmt::format("{}/particle.png", Mod::get()->getID());
        p->setTexture(CCTextureCache::sharedTextureCache()->addImage(texPath.c_str(), false));
        p->setTotalParticles(40);
        p->setDuration(0.1f);
        p->setLife(0.6f);
        p->setSpeed(140.f);
        p->setStartSize(16.f);
        p->setEndSize(2.f);
        p->setStartColor(c);
        p->setEndColor(ccc4f(c.r, c.g, c.b, 0.f));
        p->setBlendAdditive(true);
        p->setPosition(pos);
        p->setAutoRemoveOnFinish(true);
        parent->addChild(p, 100);
    }

    inline ccColor4F hueColor(float h) {
        h = fmodf(h, 360.f) / 60.f;
        int i = static_cast<int>(h);
        float fr = h - i;
        float q = 1.f - fr;
        switch (i % 6) {
            case 0: return ccc4f(1.f, fr, 0.f, 1.f);
            case 1: return ccc4f(q, 1.f, 0.f, 1.f);
            case 2: return ccc4f(0.f, 1.f, fr, 1.f);
            case 3: return ccc4f(0.f, q, 1.f, 1.f);
            case 4: return ccc4f(fr, 0.f, 1.f, 1.f);
            default: return ccc4f(1.f, 0.f, q, 1.f);
        }
    }

    // Lista de nuestros portales del nivel actual
    static std::vector<GameObject*> s_portals;
    static GJBaseGameLayer* s_scannedLayer = nullptr;

    inline void scanPortals(GJBaseGameLayer* gl) {
        s_portals.clear();
        s_scannedLayer = gl;
        if (!gl || !gl->m_objects) return;
        for (auto obj : CCArrayExt<GameObject*>(gl->m_objects)) {
            if (obj && isOurs(obj->m_objectID)) s_portals.push_back(obj);
        }
    }
}

// ---------------------------------------------------------------------------
// Crear los objetos 7001-7007
// PUNTO FRAGIL: crear objetos nuevos en GD es la parte mas delicada. Uso un
// bloque normal (id 1) como base, lo convierto en decoracion con nuestro ID y
// le pongo encima el sprite del portal.
// ---------------------------------------------------------------------------
class $modify(EVGameObject, GameObject) {
    static GameObject* createWithKey(int key) {
        if (!ev::isOurs(key)) return GameObject::createWithKey(key);

        auto obj = GameObject::createWithKey(1);
        if (!obj) return nullptr;
        obj->m_objectID = key;
        obj->m_objectType = GameObjectType::Decoration;
        obj->setOpacity(0);

        auto vis = CCSprite::createWithSpriteFrameName(ev::frameFor(key).c_str());
        if (vis) {
            vis->setScale(0.7f);
            vis->setPosition(obj->getContentSize() / 2.f);
            obj->addChild(vis, 1);
        }
        return obj;
    }
};

// ---------------------------------------------------------------------------
// Jugador: vehiculos nuevos + velocidades
// ---------------------------------------------------------------------------
class $modify(EVPlayer, PlayerObject) {
    struct Fields {
        int mode = 0;            // 0 normal, 1 pelota rebote, 2 serpiente
        bool holding = false;
        int snakeDir = 1;        // 1 sube, -1 baja
        float hue = 0.f;
        GameObject* lastPortal = nullptr;
        std::vector<CCPoint> trail;
        CCDrawNode* trailNode = nullptr;
    };

    bool inOtherMode() {
        return m_isShip || m_isBall || m_isBird || m_isDart || m_isRobot || m_isSpider || m_isSwing;
    }

    void leaveMode() {
        m_fields->mode = 0;
        m_fields->trail.clear();
        if (m_fields->trailNode) {
            m_fields->trailNode->removeFromParent();
            m_fields->trailNode = nullptr;
        }
    }

    void enterMode(int m) {
        // Volver a cubo antes de entrar a nuestro modo
        if (m_isShip)   this->toggleFlyMode(false, false);
        if (m_isBall)   this->toggleRollMode(false, false);
        if (m_isBird)   this->toggleBirdMode(false, false);
        if (m_isDart)   this->toggleDartMode(false, false);
        if (m_isRobot)  this->toggleRobotMode(false, false);
        if (m_isSpider) this->toggleSpiderMode(false, false);
        if (m_isSwing)  this->toggleSwingMode(false, false);

        leaveMode();
        m_fields->mode = m;
        m_fields->snakeDir = 1;
        if (m == 2 && this->getParent()) {
            m_fields->trailNode = CCDrawNode::create();
            this->getParent()->addChild(m_fields->trailNode, this->getZOrder() - 1);
        }
    }

    void applyPortal(GameObject* portal) {
        int id = portal->m_objectID;
        ev::playSfx(ev::soundFor(id));
        ev::burst(portal->getParent(), portal->getPosition(), ev::colorFor(id));

        if (id == ev::ID_BALL)       enterMode(1);
        else if (id == ev::ID_SNAKE) enterMode(2);
        else {
            // PUNTO FRAGIL: si la velocidad no cambia, avisame y lo ajusto.
            m_playerSpeed = ev::NORMAL_SPEED * ev::speedMultFor(id);
        }
    }

    void checkPortals() {
        auto gl = m_gameLayer;
        if (!gl) return;
        if (gl != ev::s_scannedLayer) ev::scanPortals(gl);
        if (ev::s_portals.empty()) {
            m_fields->lastPortal = nullptr;
            return;
        }
        auto prect = this->getObjectRect();
        GameObject* hit = nullptr;
        for (auto p : ev::s_portals) {
            if (p->getObjectRect().intersectsRect(prect)) {
                hit = p;
                break;
            }
        }
        if (hit && hit != m_fields->lastPortal) {
            m_fields->lastPortal = hit;
            applyPortal(hit);
        } else if (!hit) {
            m_fields->lastPortal = nullptr;
        }
    }

    void updateTrail() {
        auto& f = *m_fields;
        if (!f.trailNode) return;
        f.trail.push_back(this->getPosition());
        if (f.trail.size() > 45) f.trail.erase(f.trail.begin());

        f.trailNode->clear();
        bool rainbow = Mod::get()->getSettingValue<bool>("rainbow-trail");
        f.hue = fmodf(f.hue + 3.f, 360.f);
        size_t n = f.trail.size();
        for (size_t i = 0; i < n; i++) {
            float t = static_cast<float>(i + 1) / static_cast<float>(n);
            ccColor4F col = rainbow ? ev::hueColor(f.hue + i * 6.f) : ccc4f(0.2f, 1.f, 0.4f, 1.f);
            col.a = t;
            f.trailNode->drawDot(f.trail[i], 3.f + 6.f * t, col);
        }
    }

    bool pushButton(PlayerButton btn) {
        if (m_fields->mode != 0 && btn == PlayerButton::Jump) {
            m_fields->holding = true;
            if (m_fields->mode == 2) {
                m_fields->snakeDir = -m_fields->snakeDir;
                ev::playSfx("snake_turn.wav", 0.6f);
            }
            return true; // no saltar como cubo
        }
        return PlayerObject::pushButton(btn);
    }

    bool releaseButton(PlayerButton btn) {
        if (m_fields->mode != 0 && btn == PlayerButton::Jump) {
            m_fields->holding = false;
            return true;
        }
        return PlayerObject::releaseButton(btn);
    }

    void resetObject() {
        PlayerObject::resetObject();
        leaveMode();
        m_fields->lastPortal = nullptr;
        m_fields->holding = false;
        ev::scanPortals(m_gameLayer);
    }

    void update(float dt) {
        PlayerObject::update(dt);
        checkPortals();

        auto& f = *m_fields;
        if (f.mode == 0) return;

        // Si el jugador toca un portal normal de otro vehiculo, salimos de nuestro modo
        if (inOtherMode()) {
            leaveMode();
            return;
        }

        float flip = m_isUpsideDown ? -1.f : 1.f;

        if (f.mode == 1) {
            // Pelota Rebote: rebota sola, mantener = rebote alto
            if (m_isOnGround) {
                float low  = static_cast<float>(Mod::get()->getSettingValue<double>("bounce-low"));
                float high = static_cast<float>(Mod::get()->getSettingValue<double>("bounce-high"));
                m_yVelocity = (f.holding ? high : low) * flip;
                m_isOnGround = false;
                ev::playSfx("bounce.wav", 0.5f);
            }
            this->setRotation(this->getRotation() + 270.f * dt);
        } else if (f.mode == 2) {
            // Serpiente: diagonal constante, cada toque cambia la direccion
            float spd = static_cast<float>(Mod::get()->getSettingValue<double>("snake-speed"));
            m_yVelocity = spd * static_cast<float>(f.snakeDir) * flip;
            this->setRotation(f.snakeDir > 0 ? -35.f : 35.f);
            updateTrail();
        }
    }
};

// ---------------------------------------------------------------------------
// Editor: botones para colocar los portales + triggers faciles
// ---------------------------------------------------------------------------
class $modify(EVEditorUI, EditorUI) {
    bool init(LevelEditorLayer* lel) {
        if (!EditorUI::init(lel)) return false;

        // PUNTO FRAGIL: registrar el nombre del sprite de cada ID nuevo
        auto tb = ObjectToolbox::sharedState();
        for (int id = ev::FIRST_ID; id <= ev::LAST_ID; id++) {
            tb->m_allKeys[id] = ev::frameFor(id).c_str();
        }

        // Mini barra a la izquierda con los portales nuevos
        auto menu = CCMenu::create();
        for (int id = ev::FIRST_ID; id <= ev::LAST_ID; id++) {
            auto btn = this->getCreateBtn(id, 4);
            if (!btn) continue;
            btn->setScale(0.6f);
            menu->addChild(btn);
        }
        menu->alignItemsVerticallyWithPadding(3.f);
        auto win = CCDirector::sharedDirector()->getWinSize();
        menu->setPosition({28.f, win.height / 2.f + 20.f});
        this->addChild(menu, 100);

        return true;
    }
};

class $modify(EVEditorLayer, LevelEditorLayer) {
    // Triggers faciles: al colocar un trigger sin grupo, se le asigna el siguiente grupo
    GameObject* createObject(int key, CCPoint pos, bool undo) {
        auto obj = LevelEditorLayer::createObject(key, pos, undo);
        if (!obj || !Mod::get()->getSettingValue<bool>("auto-group")) return obj;

        if (auto eff = typeinfo_cast<EffectGameObject*>(obj)) {
            if (eff->m_isTrigger && eff->m_targetGroupID == 0) {
                int maxGroup = 0;
                for (auto o : CCArrayExt<GameObject*>(m_objects)) {
                    if (auto e2 = typeinfo_cast<EffectGameObject*>(o)) {
                        if (e2->m_targetGroupID > maxGroup) maxGroup = e2->m_targetGroupID;
                    }
                }
                eff->m_targetGroupID = maxGroup + 1;
            }
        }
        return obj;
    }
};
