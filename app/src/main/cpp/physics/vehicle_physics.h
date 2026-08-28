#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <android/log.h>

#define OBLIVION_VEHICLE_LOG_TAG "OblivionVehiclePhysics"
#define OBLIVION_VEHICLE_LOGI(...) __android_log_print(ANDROID_LOG_INFO, OBLIVION_VEHICLE_LOG_TAG, __VA_ARGS__)
#define OBLIVION_VEHICLE_LOGW(...) __android_log_print(ANDROID_LOG_WARN, OBLIVION_VEHICLE_LOG_TAG, __VA_ARGS__)

namespace oblivion {

class PhysicsManager;

// Vehicle categories supported by the physics layer.
enum class VehicleType : uint8_t {
    HORSE = 0,
    CART,
    BOAT
};

// Per-wheel/leg suspension configuration, reused for both horse "legs" and
// cart wheels since Jolt's vehicle constraint models both as wheels.
struct SuspensionConfig {
    glm::vec3 localAttachPoint{0.0f, 0.0f, 0.0f};
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    float restLength = 0.3f;
    float maxCompression = 0.15f;
    float maxExtension = 0.15f;
    float stiffness = 40.0f;
    float damping = 4.0f;
    float wheelRadius = 0.25f;
};

// Buoyancy sampling point used by boats to approximate water displacement.
struct BuoyancyPoint {
    glm::vec3 localOffset{0.0f, 0.0f, 0.0f};
    float buoyancyForce = 0.0f;
};

// Shared driving controls, applied differently depending on vehicle type.
struct VehicleControlInput {
    float throttle = 0.0f;  // -1 (reverse/back) .. 1 (forward)
    float steering = 0.0f;  // -1 (left) .. 1 (right)
    float brake = 0.0f;     // 0 .. 1
};

// A single simulated vehicle instance (horse, cart, or boat).
class VehicleInstance {
public:
    uint32_t vehicleId = 0;
    VehicleType type = VehicleType::HORSE;
    JPH::BodyID chassisBody; // horse torso / cart bed / boat hull

    std::vector<SuspensionConfig> suspensions; // 4 legs or 4 wheels
    std::vector<BuoyancyPoint> buoyancyPoints; // boats only

    // Attachment point (in chassis local space) where a rider sits.
    glm::vec3 riderAttachPoint{0.0f, 1.2f, 0.0f};

    // Attachment point for a cart being towed by this horse, or the point on
    // a cart where it connects to its horse.
    glm::vec3 towAttachPoint{0.0f, 0.5f, -1.0f};
    uint32_t towedVehicleId = 0; // 0 = none

    VehicleControlInput input;

    float currentSpeed = 0.0f;
    float maxSpeed = 8.0f;         // m/s, horse gallop default
    float acceleration = 4.0f;     // m/s^2
    float brakingDeceleration = 6.0f;
    float steeringResponse = 2.5f; // rad/s max turn rate

    // Terrain adaptation: current surface friction/slope sampled each tick.
    float currentSurfaceFriction = 0.7f;
    float currentSlopeDeg = 0.0f;

    bool isSubmerged = false; // relevant for boats fording rivers
};

// Simplified vehicle physics: horses/carts use a 4-point suspension model
// (Jolt's vehicle constraint applied to 4 "wheels", which for horses
// represent the 4 legs), boats use point-sampled buoyancy.
class VehiclePhysics {
public:
    static VehiclePhysics& getInstance() {
        static VehiclePhysics instance;
        return instance;
    }

    void init(PhysicsManager* manager) {
        physicsManager = manager;
        OBLIVION_VEHICLE_LOGI("VehiclePhysics initialized");
    }

    // Creates a horse vehicle instance with a 4-leg suspension approximation.
    uint32_t createHorse(JPH::BodyID chassisBody) {
        VehicleInstance instance;
        instance.vehicleId = nextVehicleId++;
        instance.type = VehicleType::HORSE;
        instance.chassisBody = chassisBody;
        instance.maxSpeed = 8.0f;
        instance.acceleration = 4.5f;
        instance.suspensions = defaultLegSuspensions();
        instance.riderAttachPoint = glm::vec3(0.0f, 1.1f, -0.1f);
        vehicles[instance.vehicleId] = instance;
        OBLIVION_VEHICLE_LOGI("Created horse vehicle id=%u", instance.vehicleId);
        return instance.vehicleId;
    }

    // Creates a cart vehicle instance with a standard 4-wheel suspension.
    uint32_t createCart(JPH::BodyID chassisBody) {
        VehicleInstance instance;
        instance.vehicleId = nextVehicleId++;
        instance.type = VehicleType::CART;
        instance.chassisBody = chassisBody;
        instance.maxSpeed = 5.0f;
        instance.acceleration = 2.0f;
        instance.suspensions = defaultWheelSuspensions();
        instance.riderAttachPoint = glm::vec3(0.0f, 0.9f, 0.5f);
        instance.towAttachPoint = glm::vec3(0.0f, 0.5f, 1.2f);
        vehicles[instance.vehicleId] = instance;
        OBLIVION_VEHICLE_LOGI("Created cart vehicle id=%u", instance.vehicleId);
        return instance.vehicleId;
    }

    // Creates a boat vehicle instance with buoyancy sample points.
    uint32_t createBoat(JPH::BodyID chassisBody) {
        VehicleInstance instance;
        instance.vehicleId = nextVehicleId++;
        instance.type = VehicleType::BOAT;
        instance.chassisBody = chassisBody;
        instance.maxSpeed = 6.0f;
        instance.acceleration = 1.5f;
        instance.buoyancyPoints = defaultBuoyancyPoints();
        instance.riderAttachPoint = glm::vec3(0.0f, 0.6f, 0.0f);
        vehicles[instance.vehicleId] = instance;
        OBLIVION_VEHICLE_LOGI("Created boat vehicle id=%u", instance.vehicleId);
        return instance.vehicleId;
    }

    // Attaches a cart to a horse for towing.
    void attachTow(uint32_t horseVehicleId, uint32_t cartVehicleId) {
        auto horseIt = vehicles.find(horseVehicleId);
        if (horseIt == vehicles.end()) return;
        horseIt->second.towedVehicleId = cartVehicleId;
    }

    void setControlInput(uint32_t vehicleId, const VehicleControlInput& input) {
        auto it = vehicles.find(vehicleId);
        if (it == vehicles.end()) return;
        it->second.input = input;
    }

    // Updates terrain adaptation sampling (surface friction & slope), used
    // by the driving model to reduce speed on steep/slippery terrain.
    void setTerrainSample(uint32_t vehicleId, float surfaceFriction, float slopeDeg) {
        auto it = vehicles.find(vehicleId);
        if (it == vehicles.end()) return;
        it->second.currentSurfaceFriction = surfaceFriction;
        it->second.currentSlopeDeg = slopeDeg;
    }

    // Advances the simplified driving model for all active vehicles. Actual
    // force/impulse application against JPH bodies happens in the .cpp,
    // which has access to PhysicsManager's BodyInterface.
    void update(float deltaTime) {
        for (auto& [id, vehicle] : vehicles) {
            switch (vehicle.type) {
                case VehicleType::HORSE:
                case VehicleType::CART:
                    updateGroundVehicle(vehicle, deltaTime);
                    break;
                case VehicleType::BOAT:
                    updateBoat(vehicle, deltaTime);
                    break;
            }
        }
    }

    VehicleInstance* getVehicle(uint32_t vehicleId) {
        auto it = vehicles.find(vehicleId);
        return it != vehicles.end() ? &it->second : nullptr;
    }

    void removeVehicle(uint32_t vehicleId) {
        vehicles.erase(vehicleId);
    }

    size_t getVehicleCount() const { return vehicles.size(); }

private:
    VehiclePhysics() = default;

    std::vector<SuspensionConfig> defaultLegSuspensions() const {
        std::vector<SuspensionConfig> legs(4);
        const glm::vec3 offsets[4] = {
            {0.35f, -0.4f, 0.7f}, {-0.35f, -0.4f, 0.7f},
            {0.35f, -0.4f, -0.7f}, {-0.35f, -0.4f, -0.7f}
        };
        for (int i = 0; i < 4; ++i) {
            legs[i].localAttachPoint = offsets[i];
            legs[i].restLength = 0.5f;
            legs[i].stiffness = 60.0f;
            legs[i].damping = 6.0f;
            legs[i].wheelRadius = 0.1f; // hoof contact radius approximation
        }
        return legs;
    }

    std::vector<SuspensionConfig> defaultWheelSuspensions() const {
        std::vector<SuspensionConfig> wheels(4);
        const glm::vec3 offsets[4] = {
            {0.8f, -0.3f, 1.2f}, {-0.8f, -0.3f, 1.2f},
            {0.8f, -0.3f, -1.2f}, {-0.8f, -0.3f, -1.2f}
        };
        for (int i = 0; i < 4; ++i) {
            wheels[i].localAttachPoint = offsets[i];
            wheels[i].restLength = 0.3f;
            wheels[i].stiffness = 35.0f;
            wheels[i].damping = 4.0f;
            wheels[i].wheelRadius = 0.35f;
        }
        return wheels;
    }

    std::vector<BuoyancyPoint> defaultBuoyancyPoints() const {
        std::vector<BuoyancyPoint> points(4);
        const glm::vec3 offsets[4] = {
            {0.6f, 0.0f, 1.0f}, {-0.6f, 0.0f, 1.0f},
            {0.6f, 0.0f, -1.0f}, {-0.6f, 0.0f, -1.0f}
        };
        for (int i = 0; i < 4; ++i) {
            points[i].localOffset = offsets[i];
            points[i].buoyancyForce = 0.0f;
        }
        return points;
    }

    void updateGroundVehicle(VehicleInstance& vehicle, float deltaTime) {
        // Slope-based speed reduction: steeper slopes reduce max achievable
        // speed, surface friction scales acceleration responsiveness.
        float slopeFactor = std::min(std::max(1.0f - (vehicle.currentSlopeDeg / 60.0f), 0.1f), 1.0f);
        float frictionFactor = std::min(std::max(vehicle.currentSurfaceFriction, 0.1f), 1.0f);

        float targetSpeed = vehicle.input.throttle * vehicle.maxSpeed * slopeFactor;
        float accel = vehicle.acceleration * frictionFactor;

        if (vehicle.input.brake > 0.0f) {
            float brakeAmount = vehicle.brakingDeceleration * vehicle.input.brake * deltaTime;
            if (vehicle.currentSpeed > 0.0f) {
                vehicle.currentSpeed = std::max(0.0f, vehicle.currentSpeed - brakeAmount);
            } else {
                vehicle.currentSpeed = std::min(0.0f, vehicle.currentSpeed + brakeAmount);
            }
        } else if (vehicle.currentSpeed < targetSpeed) {
            vehicle.currentSpeed = std::min(targetSpeed, vehicle.currentSpeed + accel * deltaTime);
        } else if (vehicle.currentSpeed > targetSpeed) {
            vehicle.currentSpeed = std::max(targetSpeed, vehicle.currentSpeed - accel * deltaTime);
        }

        // Steering rate scales down with speed to avoid unrealistic pivoting
        // at high velocity, similar to Oblivion's mounted-horse turning feel.
        float speedRatio = vehicle.maxSpeed > 0.0f ? std::abs(vehicle.currentSpeed) / vehicle.maxSpeed : 0.0f;
        float effectiveSteerRate = vehicle.steeringResponse * (1.0f - 0.5f * speedRatio);
        vehicle.currentSurfaceFriction = frictionFactor; // retained for next tick's blending
        (void)effectiveSteerRate; // consumed by .cpp when integrating heading
    }

    void updateBoat(VehicleInstance& vehicle, float deltaTime) {
        // Buoyancy force per sample point counteracts gravity while the
        // point is below the water plane; actual water-height sampling
        // against the world's water system happens in the .cpp.
        for (auto& point : vehicle.buoyancyPoints) {
            point.buoyancyForce = vehicle.isSubmerged ? kBuoyancyForcePerPoint : 0.0f;
        }

        float dragFactor = vehicle.isSubmerged ? kWaterDrag : kAirDrag;
        float targetSpeed = vehicle.input.throttle * vehicle.maxSpeed;
        float accel = vehicle.acceleration * (1.0f - dragFactor);

        if (vehicle.currentSpeed < targetSpeed) {
            vehicle.currentSpeed = std::min(targetSpeed, vehicle.currentSpeed + accel * deltaTime);
        } else {
            vehicle.currentSpeed = std::max(targetSpeed, vehicle.currentSpeed - accel * deltaTime);
        }
        vehicle.currentSpeed *= (1.0f - dragFactor * deltaTime);
    }

    static constexpr float kBuoyancyForcePerPoint = 250.0f;
    static constexpr float kWaterDrag = 0.35f;
    static constexpr float kAirDrag = 0.05f;

    PhysicsManager* physicsManager = nullptr;
    uint32_t nextVehicleId = 1;
    std::unordered_map<uint32_t, VehicleInstance> vehicles;
};

} // namespace oblivion
