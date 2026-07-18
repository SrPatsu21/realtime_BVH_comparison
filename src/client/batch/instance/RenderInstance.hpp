#pragma once

#include "../material/Material.hpp"
#include "BatchRegistration.hpp"
#include "../../raytracing/acceleration_structure/accelerationStructureConfig.hpp"
#include "../../raytracing/acceleration_structure/BLAS.hpp"
#include "RenderInstanceRegistration.hpp"

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

class RenderBatch;
class RenderInstanceManager;

class RenderInstance
{
    friend class RenderBatch;
    friend class RenderInstanceManager;
private:
    glm::mat4 model; //64
public:
    glm::quat rotation; //16
    glm::vec3 position; //12
    glm::vec3 scale; //12
private:
    std::vector<BatchRegistration> registrations; //12
    RenderInstanceRegistration* renderInstanceRegistration; //8
    BLAS<DefaultBLASNode>* blas; //8

public:
    // bool dirtyTransform = true; // 1

    //144 bytes -> 152

    RenderInstance(
        const glm::vec3& position = glm::vec3(0.0f),
        const glm::quat& rotation = glm::vec3(0.0f),
        const glm::vec3& scale = glm::vec3(0.0f)
    );

    ~RenderInstance();

    RenderInstance(const RenderInstance&) = delete;
    RenderInstance& operator=(const RenderInstance&) = delete;

    RenderInstance(RenderInstance&&) noexcept = delete;
    RenderInstance& operator=(RenderInstance&&) noexcept = delete;

    void updateModelMatrix();

    std::vector<BatchRegistration>& getRegistrations() { return registrations; }

private:
    void addRegistration(
        RenderBatch* batch,
        size_t index
    );

    void clearRegistrations();

public:
    const BLAS<DefaultBLASNode>* getBLAS() const
    {
        return blas;
    }

    const glm::mat4& getModelMatrix() const
    {
        return model;
    }
};