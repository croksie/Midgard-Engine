#pragma once
#include <memory>
#include <unordered_map>
#include <string>

#include <bifrost/pipeline.h>
#include <bifrost/bifrost.h>

namespace midgard::bifrost {

class PipelineManager {
public:
    std::shared_ptr<Pipeline> getOrCreatePipeline(Bifrost* rhi, std::string name, const PipelineInfo& pi) {
        auto it = m_pipelines.find(name);
        if(it != m_pipelines.cend()) {
            if (auto pipeline = it->second.lock()) {
                return pipeline;
            }
            it = m_pipelines.erase(it);
        }

        std::shared_ptr<Pipeline> pipeline = rhi->createPipeline(pi);
        m_pipelines.emplace(name, pipeline);
        return pipeline;
    }

private:
    std::unordered_map<std::string, std::weak_ptr<Pipeline>> m_pipelines;
};

} // namespace midgard::bifrost