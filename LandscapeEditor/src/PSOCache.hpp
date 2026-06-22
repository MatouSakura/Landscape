#pragma once

#include "PipelineState.h"
#include "RefCntAutoPtr.hpp"

#include <functional>
#include <string>
#include <unordered_map>

namespace Diligent
{

class PSOCache final
{
public:
    using CreatePipelineFn = std::function<RefCntAutoPtr<IPipelineState>()>;

    RefCntAutoPtr<IPipelineState> GetOrCreate(const std::string& Key, const CreatePipelineFn& CreatePipeline);

    size_t GetPSOCount() const { return m_Pipelines.size(); }
    size_t GetCreationCount() const { return m_CreationCount; }

private:
    std::unordered_map<std::string, RefCntAutoPtr<IPipelineState>> m_Pipelines;
    size_t                                                        m_CreationCount = 0;
};

} // namespace Diligent
