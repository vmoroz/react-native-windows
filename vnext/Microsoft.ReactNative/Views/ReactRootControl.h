// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include <stdint.h>
#include <memory>
#include <set>

#include <IReactInstance.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include "IXamlRootView.h"
#include "SIPEventHandler.h"
#include "TouchEventHandler.h"
#include "Views/KeyboardEventHandler.h"

namespace winrt {
using namespace Windows::UI;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml::Media;
} // namespace winrt

namespace react::uwp {

// A class that implements IXamlRootView and IXamlReactControl interfaces.
// It keeps a weak reference to the XAML ReactRootView.
// TODO: consider to remove this class in favor of ReactRootView XAML control.
struct ReactRootControl final : std::enable_shared_from_this<ReactRootControl>, IXamlRootView, IXamlReactControl {
  ReactRootControl(XamlView const &rootView) noexcept;
  ~ReactRootControl() noexcept;

 public: // IXamlRootView
  std::shared_ptr<IReactInstance> GetReactInstance() const noexcept override;
  XamlView GetXamlView() const noexcept override;
  void SetJSComponentName(std::string &&mainComponentName) noexcept override;
  void SetInstanceCreator(const ReactInstanceCreator &instanceCreator) noexcept override;
  void SetInitialProps(folly::dynamic &&initialProps) noexcept override;
  void AttachRoot() noexcept override;
  void DetachRoot() noexcept override;
  std::shared_ptr<IXamlReactControl> GetXamlReactControl() const noexcept override;

 public: // IReactRootView
  void ResetView() noexcept override;
  std::string JSComponentName() const noexcept override;
  int64_t GetActualHeight() const noexcept override;
  int64_t GetActualWidth() const noexcept override;
  int64_t GetTag() const noexcept override;
  void SetTag(int64_t tag) noexcept override;

 public: // IXamlReactControl
  void blur(XamlView const &xamlView) noexcept override;

 public:
  Mso::React::IReactViewHost *ReactViewHost() noexcept;
  void ReactViewHost(Mso::React::IReactViewHost *viewHost) noexcept;

 private:
  void PrepareXamlRootView(XamlView const &rootView) noexcept;
  void HandleInstanceError() noexcept;
  void HandleInstanceWaiting() noexcept;
  void HandleDebuggerAttach() noexcept;

  void EnsureFocusSafeHarbor() noexcept;

  void InitializeDeveloperMenu() noexcept;
  void ShowDeveloperMenu() noexcept;
  void DismissDeveloperMenu() noexcept;
  bool IsDeveloperMenuShowing() const noexcept;
  void ToggleInspector() noexcept;

  void ReloadHost() noexcept;
  void ReloadViewHost() noexcept;

  void AttachRootView() noexcept;
  void DetachRootView() noexcept;

  void ReloadUI(
      Mso::CntPtr<Mso::React::IReactInstance> const &reactInstance,
      Mso::React::ReactViewOptions &&reactViewOptions) noexcept;
  void UnloadUI() noexcept;

 private:
  int64_t m_rootTag{-1};
  Mso::DispatchQueue m_uiQueue;
  Mso::CntPtr<Mso::React::IReactViewHost> m_reactViewHost;
  Mso::WeakPtr<Mso::React::IReactInstance> m_weakReactInstance;
  std::unique_ptr<Mso::React::ReactOptions> m_reactOptions;
  std::unique_ptr<Mso::React::ReactViewOptions> m_reactViewOptions;
  std::weak_ptr<facebook::react::InstanceWrapper> m_fbReactInstance;

  bool m_useLiveReload{false};
  bool m_useWebDebugger{false};
  bool m_isRootViewAttaching{false};
  bool m_isRootViewAttached{false};
  bool m_isUILoading{false};
  bool m_isUILoaded{false};

  // Visual tree to support safe harbor
  // m_rootView
  //  safe harbor
  //  m_xamlRootView
  //    JS created children
  winrt::weak_ref<XamlView> m_weakRootView{nullptr};
  winrt::weak_ref<XamlView> m_weakXamlRootView{nullptr};

  winrt::ContentControl m_focusSafeHarbor{nullptr};
  winrt::ContentControl::LosingFocus_revoker m_focusSafeHarborLosingFocusRevoker{};
  winrt::Grid m_redBoxGrid{nullptr};
  winrt::Grid m_greenBoxGrid{nullptr};
  winrt::TextBlock m_errorTextBlock{nullptr};
  winrt::TextBlock m_waitingTextBlock{nullptr};
  winrt::Grid m_developerMenuRoot{nullptr};
  winrt::Button::Click_revoker m_remoteDebugJSRevoker{};
  winrt::Button::Click_revoker m_cancelRevoker{};
  winrt::Button::Click_revoker m_toggleInspectorRevoker{};
  winrt::Button::Click_revoker m_reloadJSRevoker{};
  winrt::Button::Click_revoker m_liveReloadRevoker{};
  winrt::Windows::UI::Core::CoreDispatcher m_uiDispatcher{nullptr};
  winrt::CoreDispatcher::AcceleratorKeyActivated_revoker m_coreDispatcherAKARevoker{};
};

//
// private:
//  IXamlRootView *m_pParent;
//
//  std::string m_jsComponentName;
//  std::shared_ptr<facebook::react::NativeModuleProvider> m_moduleProvider;
//  folly::dynamic m_initialProps;
//  std::shared_ptr<TouchEventHandler> m_touchEventHandler;
//  std::shared_ptr<SIPEventHandler> m_SIPEventHandler;
//  std::shared_ptr<PreviewKeyboardEventHandlerOnRoot> m_previewKeyboardEventHandlerOnRoot;
//
//  int64_t m_rootTag = -1;
//
//  // Visual tree to support safe harbor
//  // m_rootView
//  //  safe harbor
//  //  m_xamlRootView
//  //    JS created children
//  XamlView m_xamlRootView{nullptr};
//  XamlView m_rootView;
//
//  ReactInstanceCreator m_instanceCreator;
//  std::shared_ptr<IReactInstance> m_reactInstance;
//  bool m_isAttached{false};
//  LiveReloadCallbackCookie m_liveReloadCallbackCookie{0};
//  ErrorCallbackCookie m_errorCallbackCookie{0};
//  DebuggerAttachCallbackCookie m_debuggerAttachCallbackCookie{0};
//
//  winrt::ContentControl m_focusSafeHarbor{nullptr};
//  winrt::ContentControl::LosingFocus_revoker m_focusSafeHarborLosingFocusRevoker{};
//  winrt::Grid m_redBoxGrid{nullptr};
//  winrt::Grid m_greenBoxGrid{nullptr};
//  winrt::TextBlock m_errorTextBlock{nullptr};
//  winrt::TextBlock m_waitingTextBlock{nullptr};
//  winrt::Grid m_developerMenuRoot{nullptr};
//  winrt::Button::Click_revoker m_remoteDebugJSRevoker{};
//  winrt::Button::Click_revoker m_cancelRevoker{};
//  winrt::Button::Click_revoker m_toggleInspectorRevoker{};
//  winrt::Button::Click_revoker m_reloadJSRevoker{};
//  winrt::Button::Click_revoker m_liveReloadRevoker{};
//  winrt::Windows::UI::Core::CoreDispatcher m_uiDispatcher;
//  winrt::CoreDispatcher::AcceleratorKeyActivated_revoker m_coreDispatcherAKARevoker{};
//};

} // namespace react::uwp
