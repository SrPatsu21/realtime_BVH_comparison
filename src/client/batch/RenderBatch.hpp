#pragma once

#include "RenderInstanceManager.hpp"
#include "instance/BatchRegistration.hpp"
#include "BatchKey.hpp"
#include <vector>

class RenderBatch {
    friend class RenderInstanceManager;
private:
    BatchKey batchKey;
    std::vector<InstanceData> instancesData;
    std::vector<BatchRegistration*> batchRegistrations;
public:
    explicit RenderBatch(
        BatchKey batchKey
    );

    ~RenderBatch();

    RenderBatch(const RenderBatch& other) = delete;
    RenderBatch& operator=(const RenderBatch& other) = delete;

    RenderBatch(RenderBatch&& other) noexcept;
    RenderBatch& operator=(RenderBatch&& other) noexcept;

    void addInstance(
        RenderInstance* instance
    );

    void removeInstance(
        RenderInstance* instance,
        size_t intregistrationsIndex
    );

    bool empty();

    const BatchKey& getKey() const { return batchKey; }
    std::vector<BatchRegistration*> getRenderInstance() const{ return batchRegistrations; }
    std::vector<InstanceData>& getinstancesData() { return instancesData; }
};