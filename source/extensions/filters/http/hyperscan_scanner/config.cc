#include "source/extensions/filters/http/hyperscan_scanner/config.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace HyperscanScanner {

absl::StatusOr<Http::FilterFactoryCb> HyperscanFilterConfig::createFilterFactoryFromProto(
    const Protobuf::Message& proto_config,
    const std::string&,
    Server::Configuration::FactoryContext&) {
    
    const auto& typed_config = 
        dynamic_cast<const HyperscanScannerConfig&>(
            proto_config);

    return [config = typed_config](Http::FilterChainFactoryCallbacks& callbacks) -> void {
        auto filter = std::make_unique<HyperscanFilter>(config);
        callbacks.addStreamFilter(Http::StreamFilterSharedPtr{filter.release()});
    };
}

ProtobufTypes::MessagePtr HyperscanFilterConfig::createEmptyConfigProto() {
    return std::make_unique<HyperscanScannerConfig>();
}

/**
 * Static registration for the hyperscan filter. @see RegisterFactory.
 */
REGISTER_FACTORY(HyperscanFilterConfig,
                 Server::Configuration::NamedHttpFilterConfigFactory);

} // namespace HyperscanScanner
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy