#include "source/extensions/filters/http/hyperscan_scanner/hyperscan_scanner.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace HyperscanScanner {

HyperscanFilter::HyperscanFilter(const HyperscanScannerConfig& config) {

    ENVOY_LOG(info, "Loading {} patterns from config", config.patterns().size());
    
    try {
        // 1. 加载规则
        for (const auto& pattern : config.patterns()) {
            patterns_.push_back(pattern);
            ENVOY_LOG(trace, "Loaded pattern ID: {}, Regex: {}", pattern.id(), pattern.regex());
        }

        // 编译正则表达式
        std::vector<std::string> regex_storage;
        std::vector<const char*> expressions;
        std::vector<unsigned int> flags, ids;
        for (const auto& pattern : patterns_) {
            regex_storage.emplace_back(pattern.regex());  // 拷贝字符串
            expressions.push_back(regex_storage.back().c_str());
            flags.push_back(HS_FLAG_DOTALL | HS_FLAG_SINGLEMATCH | HS_FLAG_ALLOWEMPTY);
            ids.push_back(pattern.id());
        }
        ENVOY_LOG(info, "Prepared {} expressions for compilation", expressions.size());

        hs_compile_error_t *compile_err;
        ENVOY_LOG(info, "Starting hyperscan compilation with mode: HS_MODE_BLOCK");
        if (hs_compile_multi(expressions.data(), flags.data(), ids.data(), patterns_.size(), HS_MODE_BLOCK, nullptr, &database_, &compile_err) != HS_SUCCESS) {
            std::string err_msg = compile_err ? compile_err->message : "unknown error";
            ENVOY_LOG(error, "Hyperscan compile error: {}", err_msg);
            if (compile_err) {
                hs_free_compile_error(compile_err);
            }
            throw EnvoyException("Failed to compile Hyperscan patterns");
        }
        ENVOY_LOG(info, "Scratch space allocated successfully. Scratch ptr: {}", static_cast<void*>(scratch_));

        // 分配临时空间
        ENVOY_LOG(info, "Attempting to allocate scratch space");
        if (hs_alloc_scratch(database_, &scratch_) != HS_SUCCESS) {
            ENVOY_LOG(error, "Scratch space allocation failed. Database ptr: {}", static_cast<void*>(database_));
            hs_free_database(database_);
            throw EnvoyException("Filed to allocate Hyperscan scratch space");
        }
        ENVOY_LOG(info, "Scratch space allocated successfully. Scratch ptr: {}", static_cast<void*>(scratch_));

        // 5. 验证数据库和临时空间
        size_t db_size = 0;
        hs_database_size(database_, &db_size);
        ENVOY_LOG(info, "Hyperscan database size: {} bytes", db_size);

        size_t scratch_size = 0;
        hs_scratch_size(scratch_, &scratch_size);
        ENVOY_LOG(info, "Scratch space size: {} bytes", scratch_size);
        ENVOY_LOG(info, "Hyperscan initialized successfully");
    } catch (...) {
        ENVOY_LOG(critical, "Hyperscan initialization failed");
        throw;
    }
}

HyperscanFilter::~HyperscanFilter() {
    if (database_) hs_free_database(database_);
    if (scratch_) hs_free_scratch(scratch_);
}

// 正则匹配回调函数
int HyperscanFilter::onMatch(unsigned int id, unsigned long long from,
                        unsigned long long to, unsigned int flags, void* ctx) {
    UNREFERENCED_PARAMETER(flags);

    auto *filter = static_cast<HyperscanFilter *>(ctx);
    
    // 查找匹配的规则
    auto it = std::find_if(filter->patterns_.begin(), filter->patterns_.end(),
                          [id](const Pattern& p) { return p.id() == id; });
    
    if (it != filter->patterns_.end()) {
        ENVOY_LOG(warn, "Hyperscan matched rule {} ({}), position {}-{}", 
                 id, it->description(), from, to);

        // 根据规则配置的动作执行相应操作
        switch (it->action()) {
            case Pattern::BLOCK:
                filter->callbacks_->sendLocalReply(
                    Http::Code::Forbidden,
                    "Blocked by security policy",
                    nullptr,
                    absl::nullopt,
                    it->description());
                return HS_SCAN_TERMINATED;
            
            case Pattern::LOG:
                // 仅记录日志,继续处理
                break;
                
            case Pattern::CONTINUE:
            default:
                break;
        }
    }
    
    return 0;
}
    
Http::FilterHeadersStatus HyperscanFilter::decodeHeaders(Http::RequestHeaderMap& headers, bool end_stream) {
    UNREFERENCED_PARAMETER(end_stream);
    // 检查URL路径
    absl::string_view path = headers.getPathValue();
    ENVOY_LOG(info, "decodeHeaders, path={}", path);
    if (hs_scan(database_, path.data(), path.length(), 0, scratch_, onMatch, this) != HS_SUCCESS) {
        ENVOY_LOG(error, "Hyperscan scan failed on URL");
    }
    return Http::FilterHeadersStatus::Continue;
}


Http::FilterDataStatus HyperscanFilter::decodeData(Buffer::Instance& data, bool end_stream) {
    UNREFERENCED_PARAMETER(end_stream);
    // 检查请求body
    const uint64_t length = data.length();
    auto raw_data = std::make_unique<char[]>(length);
    data.copyOut(0, length, raw_data.get());

    ENVOY_LOG(info, "decodeData, length={}, data={}", length, data.toString());
    if (hs_scan(database_, raw_data.get(), length, 0, scratch_, onMatch, this) != HS_SUCCESS) {
        ENVOY_LOG(error, "Hyperscan scan failed on body");
    }
    
    return Http::FilterDataStatus::Continue;
}


} // end namespace HyperscanScanner
} // end namespace HttpFilters
} // end namespace Extensions
} // end namespace Envoy