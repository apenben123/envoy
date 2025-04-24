#pragma once

#include "envoy/registry/registry.h"
#include "envoy/server/filter_config.h"

#include "source/extensions/filters/http/hyperscan_scanner/hyperscan_scanner.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace HyperscanScanner {

// Hyperscan 过滤器工厂
class HyperscanFilterFactory : public Server::Configuration::NamedHttpFilterConfigFactory {
public:
    absl::StatusOr<Http::FilterFactoryCb> createFilterFactoryFromProto(
        const Protobuf::Message& proto_config,
        const std::string& stats_prefix,
        Server::Configuration::FactoryContext& context) override;

    ProtobufTypes::MessagePtr createEmptyConfigProto() override;

    /* 过滤器的唯一标识
        在 Envoy 的过滤器生态系统里，每个过滤器都需要有一个唯一的名称。
        name() 方法返回的字符串就充当了这个过滤器的标识符。
        例如，在配置文件中，你可以通过这个名称来引用和启用特定的过滤器。
    */
    std::string name() const override { return "hyperscan_scanner"; }
};

} // namespace HyperscanScanner
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy