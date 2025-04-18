#pragma once

#include "envoy/registry/registry.h"
#include "envoy/server/filter_config.h"

#include "source/extensions/filters/http/hyperscan_scanner/hyperscan_scanner.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace HyperscanScanner {

class HyperscanFilterConfig : public Server::Configuration::NamedHttpFilterConfigFactory {
public:
    absl::StatusOr<Http::FilterFactoryCb> createFilterFactoryFromProto(
        const Protobuf::Message& proto_config,
        const std::string& stats_prefix,
        Server::Configuration::FactoryContext& context) override;

    ProtobufTypes::MessagePtr createEmptyConfigProto() override;

    std::string name() const override { return "hyperscan_scanner"; }
};

} // namespace HyperscanScanner
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy