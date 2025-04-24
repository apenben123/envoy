#pragma once

#include <hs/hs.h>
#include <memory>  // 添加 std::make_unique 支持
#include "envoy/http/filter.h"
#include "envoy/http/header_map.h"
#include "envoy/buffer/buffer.h"
#include "source/common/common/logger.h"  // 添加日志支持
#include "envoy/extensions/filters/http/hyperscan_scanner/v3/hyperscan_scanner.pb.h"

using envoy::extensions::filters::http::hyperscan_scanner::v3::HyperscanScannerConfig;
using envoy::extensions::filters::http::hyperscan_scanner::v3::Pattern;


namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace HyperscanScanner {

class HyperscanFilter : public Http::StreamFilter,
                        public Logger::Loggable<Logger::Id::filter> {
public:
    HyperscanFilter(const HyperscanScannerConfig& config);
    ~HyperscanFilter();



    void onDestroy() override {}

    // 处理请求头和URL
    Http::FilterHeadersStatus decodeHeaders(Http::RequestHeaderMap& headers, bool end_stream) override;
    // 处理请求Body
    Http::FilterDataStatus decodeData(Buffer::Instance& data, bool end_stream) override;
    void setDecoderFilterCallbacks(Http::StreamDecoderFilterCallbacks& callbacks) override {
        callbacks_ = &callbacks;
    }

    // 其他必要方法（空实现）
    Http::FilterTrailersStatus decodeTrailers(Http::RequestTrailerMap& trailers) override {
        UNREFERENCED_PARAMETER(trailers);
        return Http::FilterTrailersStatus::Continue;
    }

    /**
     * Called with headers to be encoded, optionally indicating end of stream.
     *
     * The only 1xx that may be provided to encodeHeaders() is a 101 upgrade, which will be the final
     * encodeHeaders() for a response.
     *
     * @param headers supplies the headers to be encoded.
     * @param end_stream supplies whether this is a header only request/response.
     * @return FilterHeadersStatus determines how filter chain iteration proceeds.
     */
    Http::FilterHeadersStatus encodeHeaders(Http::ResponseHeaderMap&, bool) override {
        return Http::FilterHeadersStatus::Continue;
    }

    Http::Filter1xxHeadersStatus encode1xxHeaders(Http::ResponseHeaderMap&) override {
        return Http::Filter1xxHeadersStatus::Continue;
    }
    Http::FilterMetadataStatus encodeMetadata(Http::MetadataMap&) override {
        return Http::FilterMetadataStatus::Continue;
    }
    /**
     * Called with data to be encoded, optionally indicating end of stream.
     * @param data supplies the data to be encoded.
     * @param end_stream supplies whether this is the last data frame.
     * Further note that end_stream is only true if there are no trailers.
     * @return FilterDataStatus determines how filter chain iteration proceeds.
     */
    Http::FilterDataStatus encodeData(Buffer::Instance&, bool) override {
        return Http::FilterDataStatus::Continue;
    }

    /**
     * Called with trailers to be encoded, implicitly ending the stream.
     * @param trailers supplies the trailers to be encoded.
     */
    Http::FilterTrailersStatus encodeTrailers(Http::ResponseTrailerMap&) override {
        return Http::FilterTrailersStatus::Continue;
    }

    void setEncoderFilterCallbacks(Http::StreamEncoderFilterCallbacks&) override {
    }

private:
    // Hyperscan 数据库和临时空间
    hs_database_t* database_{nullptr};
    hs_scratch_t* scratch_{nullptr};

    // 正则匹配回调函数
    static int onMatch(unsigned int id, unsigned long long from,
                        unsigned long long to, unsigned int flags, void* ctx);
                    
    // Envoy 回调接口
    Http::StreamDecoderFilterCallbacks* callbacks_{nullptr};

    // 从配置加载的规则
    std::vector<Pattern> patterns_;
    
}; // class HyperscanFilter


} // namespace HyperscanScanner
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy