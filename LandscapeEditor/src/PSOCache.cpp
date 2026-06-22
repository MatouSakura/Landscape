#include "PSOCache.hpp"

#include "Errors.hpp"

namespace Diligent
{

RefCntAutoPtr<IPipelineState> PSOCache::GetOrCreate(const std::string& Key, const CreatePipelineFn& CreatePipeline)
{
    auto Existing = m_Pipelines.find(Key);
    if (Existing != m_Pipelines.end())
        return Existing->second;

    RefCntAutoPtr<IPipelineState> Pipeline = CreatePipeline();
    VERIFY_EXPR(Pipeline);

    auto Inserted = m_Pipelines.emplace(Key, Pipeline);
    VERIFY_EXPR(Inserted.second);
    ++m_CreationCount;
    return Inserted.first->second;
}

} // namespace Diligent
