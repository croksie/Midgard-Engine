#pragma once
#include <memory>
#include <unordered_map>
#include <string>

#include <bifrost/pipeline.h>
#include <bifrost/bifrost.h>

namespace midgard::bifrost {

class PipelineManager {
public:
    std::shared_ptr<Pipeline> getPipeline(const std::string& name) {
        auto it = m_pipelines.find(name);
        if (it != m_pipelines.cend()) {
            if (auto pipeline = it->second.lock()) {
                return pipeline;
            }
            m_pipelines.erase(it);
        }
        return nullptr;
    }

    std::shared_ptr<Pipeline> getOrCreatePipeline(Bifrost* rhi, const std::string& name, const PipelineInfo& pi) {
        if (auto pipeline = getPipeline(name)) {
            return pipeline;
        }

        std::shared_ptr<Pipeline> pipeline = rhi->createPipeline(pi);
        m_pipelines.emplace(name, pipeline);
        return pipeline;
    }

private:
    std::unordered_map<std::string, std::weak_ptr<Pipeline>> m_pipelines;
};

} // namespace midgard::bifrost