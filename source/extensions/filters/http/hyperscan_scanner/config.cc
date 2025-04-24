#include "source/extensions/filters/http/hyperscan_scanner/config.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace HyperscanScanner {

absl::StatusOr<Http::FilterFactoryCb> HyperscanFilterFactory::createFilterFactoryFromProto(
    const Protobuf::Message& proto_config,
    const std::string&,
    Server::Configuration::FactoryContext&) {

    // dynamic_cast  主要用于在继承体系中进行安全的向下转型（从基类指针或引用转换为派生类指针或引用），
    // 同时也可用于交叉转型（在不同的派生类之间进行转换）
    // Derived& derivedRef = dynamic_cast<Derived&>(baseRef);
    // Derived* derivedPtr = dynamic_cast<Derived*>(basePtr);
    const auto& typed_config = dynamic_cast<const HyperscanScannerConfig&>(proto_config);

    return [config = typed_config](Http::FilterChainFactoryCallbacks& callbacks) -> void {
        // 构建 HyperscanFilter
        auto filter = std::make_unique<HyperscanFilter>(config);
        callbacks.addStreamFilter(Http::StreamFilterSharedPtr{filter.release()});
    };
}

ProtobufTypes::MessagePtr HyperscanFilterFactory::createEmptyConfigProto() {
    return std::make_unique<HyperscanScannerConfig>();
}

/**
 * Static registration for the hyperscan filter. @see RegisterFactory.
 * 都要调用这个 REGISTER_FACTORY 进行注册
 */
REGISTER_FACTORY(HyperscanFilterFactory,
                 Server::Configuration::NamedHttpFilterConfigFactory);

} // namespace HyperscanScanner
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy