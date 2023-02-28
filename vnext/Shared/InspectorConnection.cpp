// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "InspectorConnection.h"

namespace Microsoft::ReactNative {

namespace {

class InspectorConnectionImpl : public InspectorConnection {
 public:
  static InspectorConnectionImpl &getInstance() noexcept {
    InspectorConnectionImpl instance;
    return instance;
  }

  void registerConnectionCreator(const std::string &vm, const ConnectionCreator &connector) noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_vmToConnectionCreator[vm] = connector;
  }

  ConnectionCreator getConnectionCreator(const std::string &vm) noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_vmToConnectionCreator.find(vm);
    if (it != m_vmToConnectionCreator.end()) {
      return it->second;
    }
    return ConnectionCreator{};
  }

 private:
  mutable std::mutex m_mutex;
  std::unordered_map<std::string, ConnectionCreator> m_vmToConnectionCreator;
};

} // namespace

std::unique_ptr<facebook::react::ILocalConnection> InspectorConnection::connect(
    int32_t pageId,
    std::unique_ptr<facebook::react::IRemoteConnection> remoteConnection) noexcept {
  auto inspectorPages = facebook::react::getInspectorInstance().getPages();
  auto it =
      std::find_if(inspectorPages.begin(), inspectorPages.end(), [pageId](const facebook::react::InspectorPage &page) {
        return page.id == pageId;
      });
  if (it == inspectorPages.end()) {
    return nullptr;
  }

  ConnectionCreator creator = InspectorConnectionImpl::getInstance().getConnectionCreator(it->vm);
  if (!creator) {
    return nullptr;
  }

  return creator(pageId, std::move(remoteConnection));
}

void InspectorConnection::registerConnectionCreator(
    const std::string &vm,
    const ConnectionCreator &connector) noexcept {
  InspectorConnectionImpl::getInstance().registerConnectionCreator(vm, connector);
}

} // namespace Microsoft::ReactNative
