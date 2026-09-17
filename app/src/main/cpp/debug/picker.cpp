// Phase 64 Debug: NPC Picker Implementation

#include "picker.h"
#include "../collision/collision_world.h"
#include "../world/world_entity.h"
#include "../engine/camera.h"
#include "../ui/text_renderer.h"
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace pk {

static bool        armed       = false;
static uint32_t    npc         = 0;
static std::string label;        // nifPath file name part
static glm::vec3   pos;          // WorldEntity::position
static glm::vec3   rot;          // WorldEntity::rotation
static bool        active       = false; // WorldEntity::isActive
static bool        visible      = false; // WorldEntity::isVisible
static bool        has          = false;

void set_armed(bool a) { armed = a; }
bool  has_pick()       { return has; }
uint32_t id()          { return npc; }

static std::string file_name(const std::string& path) {
    auto p = path.find_last_of('/');
    return (p == std::string::npos) ? path : path.substr(p + 1);
}

void tap(const Camera& cam, CollisionWorld& coll,
         WorldLoader& loader, float sx, float sy) {
    if (!armed) return;
    armed = false;
    has   = false;

    float nx = sx * 2.f - 1.f;
    float ny = 1.f - sy * 2.f;

    glm::vec3 fwd   = cam.getForward();
    glm::vec3 right = glm::normalize(glm::cross(fwd, glm::vec3(0,1,0)));
    glm::vec3 up    = glm::cross(right, fwd);
    float   tanFov  = std::tan(25.f * 3.14159f / 180.f);
    glm::vec3 dir   = glm::normalize(fwd + right*nx*tanFov + up*ny*tanFov);
    glm::vec3 orig  = cam.getPosition();

    std::vector<int32_t> hits;
    coll.raycast(orig, dir, 300.f, hits);

    for (int32_t bid : hits) {
        auto it = coll.bodyToNpc.find(bid);
        if (it == coll.bodyToNpc.end()) continue;
        uint32_t nid = it->second;
        auto* e = loader.getEntityByNpcId(nid);
        if (!e) continue;

        npc     = nid;
        label   = file_name(e->nifPath);
        pos     = e->position;
        rot     = e->rotation;
        active  = e->isActive;
        visible = e->isVisible;
        has = true;
        return;
    }
}

void teleport(Camera& cam, WorldLoader& loader) {
    if (!has) return;
    // PLAYER_NPC_ID should be defined in the project
    auto* pl = loader.getEntityByNpcId(1); // placeholder
    if (!pl) return;
    pl->position = pos;
    cam.setPosition(pos + glm::vec3(0, 2.f, 0));
    cam.rotate(0.f, 0.f);
}

void info(TextRenderer& tr, float x, float y) {
    if (!has) return;
    char b[192];
    glm::vec3 cyan  = {0.1f,0.85f,0.95f};
    glm::vec3 grey  = {0.7f,0.7f,0.7f};
    glm::vec3 amber = {1.f,0.8f,0.2f};
    glm::vec3 dim   = {0.5f,0.5f,0.5f};
    glm::vec3 green = {0.3f,0.9f,0.4f};

    snprintf(b,sizeof(b),"%s [%u]", label.c_str(), npc);
    tr.renderText(b, x, y, cyan, 1.1f);

    snprintf(b,sizeof(b),"pos(%.1f, %.1f, %.1f)", pos.x,pos.y,pos.z);
    tr.renderText(b, x, y - 0.022f, grey, 0.9f);

    snprintf(b,sizeof(b),"rot(%.1f, %.1f, %.1f)", rot.x,rot.y,rot.z);
    tr.renderText(b, x, y - 0.044f, amber, 0.9f);

    snprintf(b,sizeof(b),"%s %s",
             active  ? "active "  : "",
             visible ? "visible"  : "");
    tr.renderText(b, x, y - 0.066f,
                  active ? green : dim, 0.85f);

    snprintf(b,sizeof(b),"npc=%u", npc);
    tr.renderText(b, x, y - 0.088f, dim, 0.8f);

    tr.renderText("[F]follow [T]teleport", x, y + 0.024f, grey, 0.8f);
}

} // namespace pk
