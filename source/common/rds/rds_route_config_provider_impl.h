#pragma once

#include <memory>
#include <string>

#include "envoy/rds/route_config_provider.h"
#include "envoy/rds/route_config_update_receiver.h"
#include "envoy/server/factory_context.h"
#include "envoy/thread_local/thread_local.h"

#include "source/common/rds/rds_route_config_subscription.h"

namespace Envoy {
namespace Rds {

/**
 * 路由配置提供者
 * RouteConfigProvider 的实现类，该类利用订阅机制动态获取路由配置 
 */
class RdsRouteConfigProviderImpl : public RouteConfigProvider,
                                   Logger::Loggable<Logger::Id::router> {
public:
  RdsRouteConfigProviderImpl(RdsRouteConfigSubscriptionSharedPtr&& subscription,
                             Server::Configuration::ServerFactoryContext& factory_context);

  ~RdsRouteConfigProviderImpl() override;

  RdsRouteConfigSubscription& subscription() { return *subscription_; }

  // RouteConfigProvider
  ConfigConstSharedPtr config() const override { return tls_->config_; }

  const absl::optional<ConfigInfo>& configInfo() const override;
  SystemTime lastUpdated() const override { return config_update_info_->lastUpdated(); }
  absl::Status onConfigUpdate() override;

private:
  struct ThreadLocalConfig : public ThreadLocal::ThreadLocalObject {
    ThreadLocalConfig(std::shared_ptr<const Config> initial_config)
        : config_(std::move(initial_config)) {}
    ConfigConstSharedPtr config_;
  };

  RdsRouteConfigSubscriptionSharedPtr subscription_;  //路由配置订阅者, 向控制面订阅资源
  RouteConfigUpdatePtr& config_update_info_;          //路由配置接受者, 负责接收控制面发来的资源
  ThreadLocal::TypedSlot<ThreadLocalConfig> tls_;
};

} // namespace Rds
} // namespace Envoy
