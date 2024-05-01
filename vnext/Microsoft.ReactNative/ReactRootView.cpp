// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ReactRootView.h"
#include "ReactRootView.g.cpp"

#include <QuirkSettings.h>
#include <ReactHost/DebuggerNotifications.h>
#include <ReactHost/MsoUtils.h>
#include <UI.Text.h>
#include <UI.Xaml.Controls.Primitives.h>
#include <UI.Xaml.Input.h>
#include <UI.Xaml.Media.Media3D.h>
#include <Utils/Helpers.h>
#include <dispatchQueue/dispatchQueue.h>
#include <winrt/Windows.UI.Core.h>
#include "InstanceManager.h"
#include "ReactNativeHost.h"
#include "ReactViewInstance.h"
#include "Utils/KeyboardUtils.h"
#include "XamlUtils.h"

#include <winrt/Microsoft.UI.Xaml.Controls.h>

#ifdef USE_FABRIC
#include <Fabric/FabricUIManagerModule.h>
#include <react/renderer/core/LayoutConstraints.h>
#include <react/renderer/core/LayoutContext.h>
#endif

namespace winrt::Microsoft::ReactNative::implementation {

ReactRootView::ReactRootView() noexcept : m_uiQueue(Mso::DispatchQueue::GetCurrentUIThreadQueue()) {
  VerifyElseCrashSz(m_uiQueue, "Cannot get UI dispatch queue for the current thread");

  m_xamlRootView = winrt::Grid{};
  Children().Append(m_xamlRootView);

  UpdatePerspective();
  Loaded([this](auto &&, auto &&) {
    ::Microsoft::ReactNative::SetCompositor(::Microsoft::ReactNative::GetCompositor(*this));
    SetupDevToolsShortcut();
  });
}

ReactNative::ReactNativeHost ReactRootView::ReactNativeHost() noexcept {
  return m_reactNativeHost;
}

void ReactRootView::ReactNativeHost(ReactNative::ReactNativeHost const &value) noexcept {
  if (m_reactNativeHost != value) {
    ReactViewHost(nullptr);
    m_reactNativeHost = value;
    const auto weakThis = this->get_weak();
    ::Microsoft::ReactNative::DebuggerNotifications::SubscribeShowDebuggerPausedOverlay(
        m_reactNativeHost.InstanceSettings().Notifications(),
        m_reactNativeHost.InstanceSettings().UIDispatcher(),
        [weakThis](std::string message, std::function<void()> onResume) {
          if (auto strongThis = weakThis.get()) {
            strongThis->ShowDebuggerPausedOverlay(message, onResume);
          }
        },
        [weakThis]() {
          if (auto strongThis = weakThis.get()) {
            strongThis->HideDebuggerPausedOverlay();
          }
        });
    ReloadView();
  }
}

winrt::hstring ReactRootView::ComponentName() noexcept {
  return m_componentName;
}

void ReactRootView::ComponentName(winrt::hstring const &value) noexcept {
  if (m_componentName != value) {
    m_componentName = value;
    ReloadView();
  }
}

ReactNative::JSValueArgWriter ReactRootView::InitialProps() noexcept {
  return m_initialPropsWriter;
}

void ReactRootView::InitialProps(ReactNative::JSValueArgWriter const &value) noexcept {
  if (m_initialPropsWriter != value) {
    m_initialPropsWriter = value;
    ReloadView();
  }
}

void ReactRootView::ReloadView() noexcept {
  if (m_reactNativeHost && !m_componentName.empty()) {
    Mso::React::ReactViewOptions viewOptions{};
    viewOptions.ComponentName = to_string(m_componentName);
    viewOptions.InitialProps = m_initialPropsWriter;
    if (auto reactViewHost = ReactViewHost()) {
      reactViewHost->ReloadViewInstanceWithOptions(std::move(viewOptions));
    } else {
      auto reactNativeHost = winrt::get_self<implementation::ReactNativeHost>(m_reactNativeHost);
      auto newReactViewHost = reactNativeHost->ReactHost()->MakeViewHost(std::move(viewOptions));
      ReactViewHost(newReactViewHost.Get());
    }
  } else {
    ReactViewHost(nullptr);
  }
}

void ReactRootView::UpdatePerspective() {
  // Xaml's default projection in 3D is orthographic (all lines are parallel)
  // However React Native's default projection is a one-point perspective.
  // Set a default perspective projection on the main control to mimic this.
  if (m_isPerspectiveEnabled) {
    auto perspectiveTransform3D = xaml::Media::Media3D::PerspectiveTransform3D();
    perspectiveTransform3D.Depth(850);
    xaml::Media::Media3D::Transform3D t3d(perspectiveTransform3D);
    m_xamlRootView.Transform3D(t3d);
  } else {
    m_xamlRootView.ClearValue(xaml::UIElement::Transform3DProperty());
  }
}

::Microsoft::ReactNative::XamlView ReactRootView::GetXamlView() const noexcept {
  return ::Microsoft::ReactNative::XamlView(*this);
}

std::string ReactRootView::JSComponentName() const noexcept {
  return to_string(m_componentName);
}

int64_t ReactRootView::GetActualHeight() const noexcept {
  return static_cast<int64_t>(m_xamlRootView.ActualHeight());
}

int64_t ReactRootView::GetActualWidth() const noexcept {
  return static_cast<int64_t>(m_xamlRootView.ActualWidth());
}

int64_t ReactRootView::GetTag() const noexcept {
  return m_rootTag;
}

void ReactRootView::SetTag(int64_t tag) noexcept {
  m_rootTag = tag;
}

// Xaml doesn't provide Blur.
// If 'focus safe harbor' exists, make harbor to allow tabstop and focus on
// harbor with ::Pointer otherwise, just changing the FocusState to ::Pointer
// for the element.
void ReactRootView::blur(::Microsoft::ReactNative::XamlView const &xamlView) noexcept {
  EnsureFocusSafeHarbor();
  if (m_focusSafeHarbor) {
    m_focusSafeHarbor.IsTabStop(true);
    xaml::Input::FocusManager::TryFocusAsync(m_focusSafeHarbor, winrt::FocusState::Pointer);
  } else
    xaml::Input::FocusManager::TryFocusAsync(xamlView, winrt::FocusState::Pointer);
}

void ReactRootView::InitRootView(
    Mso::CntPtr<Mso::React::IReactInstance> &&reactInstance,
    Mso::React::ReactViewOptions &&reactViewOptions) noexcept {
  VerifyElseCrash(m_uiQueue.HasThreadAccess());

  if (m_isInitialized) {
    UninitRootView();
  }

  m_reactOptions = std::make_unique<Mso::React::ReactOptions>(reactInstance->Options());
  m_weakReactInstance = Mso::WeakPtr{reactInstance};
  m_context = &reactInstance->GetReactContext();
  m_reactViewOptions = std::make_unique<Mso::React::ReactViewOptions>(std::move(reactViewOptions));

  m_touchEventHandler = std::make_shared<::Microsoft::ReactNative::TouchEventHandler>(*m_context);
  m_SIPEventHandler = std::make_shared<::Microsoft::ReactNative::SIPEventHandler>(*m_context);
  m_previewKeyboardEventHandlerOnRoot =
      std::make_shared<::Microsoft::ReactNative::PreviewKeyboardEventHandlerOnRoot>(*m_context);

  m_touchEventHandler->AddTouchHandlers(*this);
  m_previewKeyboardEventHandlerOnRoot->hook(*this);
  m_SIPEventHandler->AttachView(*this, /*fireKeyboardEvents:*/ true);

  UpdateRootViewInternal();
  AttachBackHandlers();

  m_isInitialized = true;
}

void ReactRootView::UpdateRootView() noexcept {
  VerifyElseCrash(m_uiQueue.HasThreadAccess());
  VerifyElseCrash(m_isInitialized);
  UpdateRootViewInternal();
}

void ReactRootView::UpdateRootViewInternal() noexcept {
  if (auto reactInstance = m_weakReactInstance.GetStrongPtr()) {
    switch (reactInstance->State()) {
      case Mso::React::ReactInstanceState::Loading:
        ShowInstanceLoading();
        break;
      case Mso::React::ReactInstanceState::WaitingForDebugger:
        ShowInstanceWaiting();
        break;
      case Mso::React::ReactInstanceState::Loaded:
        ShowInstanceLoaded();
        break;
      case Mso::React::ReactInstanceState::HasError:
        ShowInstanceError();
        break;
      default:
        VerifyElseCrashSz(false, "Unexpected value");
    }
  }
}

void ReactRootView::UninitRootView() noexcept {
  if (!m_isInitialized) {
    return;
  }

  if (m_isJSViewAttached) {
    if (auto reactInstance = m_weakReactInstance.GetStrongPtr()) {
      reactInstance->DetachRootView(this, false);
    }
  }

  if (m_touchEventHandler) {
    m_touchEventHandler->RemoveTouchHandlers();
  }

  if (m_previewKeyboardEventHandlerOnRoot) {
    m_previewKeyboardEventHandlerOnRoot->unhook();
  }

  RemoveBackHandlers();

  // Clear members with a dependency on the reactInstance
  m_touchEventHandler.reset();
  m_SIPEventHandler.reset();

  m_rootTag = -1;
  m_reactOptions = nullptr;
  m_context.Clear();
  m_reactViewOptions = nullptr;
  m_weakReactInstance = nullptr;

  m_isInitialized = false;
}

void ReactRootView::ClearLoadingUI() noexcept {
  if (m_xamlRootView) {
    auto children = m_xamlRootView.Children();
    uint32_t index{0};
    if (m_greenBoxGrid && children.IndexOf(m_greenBoxGrid, index)) {
      children.RemoveAt(index);
    }
  }
}

void ReactRootView::EnsureLoadingUI() noexcept {
  if (m_xamlRootView) {
    // Create Grid & TextBlock to hold text
    if (m_waitingTextBlock == nullptr) {
      m_waitingTextBlock = winrt::TextBlock();

      m_greenBoxGrid = winrt::Grid{};
      auto c = xaml::Controls::ColumnDefinition{};
      m_greenBoxGrid.ColumnDefinitions().Append(c);
      c = xaml::Controls::ColumnDefinition{};
      c.Width(xaml::GridLengthHelper::Auto());
      m_greenBoxGrid.ColumnDefinitions().Append(c);
      c = xaml::Controls::ColumnDefinition{};
      c.Width(xaml::GridLengthHelper::Auto());
      m_greenBoxGrid.ColumnDefinitions().Append(c);
      c = xaml::Controls::ColumnDefinition{};
      m_greenBoxGrid.ColumnDefinitions().Append(c);

      m_waitingTextBlock.SetValue(xaml::Controls::Grid::ColumnProperty(), winrt::box_value(1));
      m_greenBoxGrid.Background(xaml::Media::SolidColorBrush(xaml::FromArgb(0x80, 0x03, 0x29, 0x29)));
      m_greenBoxGrid.Children().Append(m_waitingTextBlock);
      m_greenBoxGrid.VerticalAlignment(xaml::VerticalAlignment::Center);
      Microsoft::UI::Xaml::Controls::ProgressRing ring{};
      ring.SetValue(xaml::Controls::Grid::ColumnProperty(), winrt::box_value(2));
      ring.IsActive(true);
      m_greenBoxGrid.Children().Append(ring);

      // Format TextBlock
      m_waitingTextBlock.TextAlignment(winrt::TextAlignment::Center);
      m_waitingTextBlock.TextWrapping(xaml::TextWrapping::Wrap);
      m_waitingTextBlock.FontFamily(winrt::FontFamily(L"Segoe UI"));
      m_waitingTextBlock.Foreground(xaml::Media::SolidColorBrush(winrt::Colors::White()));
      winrt::Thickness margin = {10.0f, 10.0f, 10.0f, 10.0f};
      m_waitingTextBlock.Margin(margin);
    }

    auto children = m_xamlRootView.Children();
    uint32_t index;
    if (m_greenBoxGrid && !children.IndexOf(m_greenBoxGrid, index)) {
      children.Append(m_greenBoxGrid);
    }
  }
}

void ReactRootView::HideDebuggerPausedOverlay() noexcept {
  m_isDebuggerPausedOverlayOpen = false;
  if (m_debuggerPausedFlyout) {
    m_debuggerPausedFlyout.Hide();
    m_debuggerPausedFlyout = nullptr;
  }
}

void ReactRootView::ShowDebuggerPausedOverlay(
    const std::string &message,
    const std::function<void()> &onResume) noexcept {
  // Initialize content
  const xaml::Controls::Grid contentGrid;
  xaml::Controls::ColumnDefinition messageColumnDefinition;
  xaml::Controls::ColumnDefinition buttonColumnDefinition;
  messageColumnDefinition.MinWidth(60);
  buttonColumnDefinition.MinWidth(36);
  contentGrid.ColumnDefinitions().Append(messageColumnDefinition);
  contentGrid.ColumnDefinitions().Append(buttonColumnDefinition);
  xaml::Controls::TextBlock messageBlock;
  messageBlock.Text(winrt::to_hstring(message));
  messageBlock.FontWeight(winrt::Windows::UI::Text::FontWeights::SemiBold());
  xaml::Controls::FontIcon resumeGlyph;
  resumeGlyph.FontFamily(xaml::Media::FontFamily(L"Segoe MDL2 Assets"));
  resumeGlyph.Foreground(xaml::Media::SolidColorBrush(winrt::Colors::Green()));
  resumeGlyph.Glyph(L"\uF5B0");
  resumeGlyph.HorizontalAlignment(xaml::HorizontalAlignment::Right);
  resumeGlyph.PointerReleased([onResume](auto &&...) { onResume(); });
  xaml::Controls::Grid::SetColumn(resumeGlyph, 1);
  contentGrid.Children().Append(messageBlock);
  contentGrid.Children().Append(resumeGlyph);

  // Configure flyout
  m_isDebuggerPausedOverlayOpen = true;
  xaml::Style flyoutStyle(
      {XAML_NAMESPACE_STR L".Controls.FlyoutPresenter", winrt::Windows::UI::Xaml::Interop::TypeKind::Metadata});
  flyoutStyle.Setters().Append(winrt::Setter(
      xaml::Controls::Control::CornerRadiusProperty(), winrt::box_value(xaml::CornerRadius{12, 12, 12, 12})));
  flyoutStyle.Setters().Append(winrt::Setter(
      xaml::Controls::Control::BackgroundProperty(),
      winrt::box_value(xaml::Media::SolidColorBrush{FromArgb(255, 255, 255, 193)})));
  flyoutStyle.Setters().Append(
      winrt::Setter(xaml::FrameworkElement::MarginProperty(), winrt::box_value(xaml::Thickness{0, 12, 0, 0})));
  m_debuggerPausedFlyout = xaml::Controls::Flyout{};
  m_debuggerPausedFlyout.FlyoutPresenterStyle(flyoutStyle);
  m_debuggerPausedFlyout.LightDismissOverlayMode(xaml::Controls::LightDismissOverlayMode::On);
  m_debuggerPausedFlyout.Content(contentGrid);

  // Disable light dismiss
  m_debuggerPausedFlyout.Closing([weakThis = this->get_weak()](auto &&, const auto &args) {
    if (auto strongThis = weakThis.get()) {
      args.Cancel(strongThis->m_isDebuggerPausedOverlayOpen);
    }
  });

  // Show flyout
  m_debuggerPausedFlyout.ShowAt(*this);
}

void ReactRootView::ShowInstanceLoaded() noexcept {
  if (m_xamlRootView) {
    ClearLoadingUI();

    if (auto reactInstance = m_weakReactInstance.GetStrongPtr()) {
      reactInstance->AttachMeasuredRootView(this, Mso::Copy(m_reactViewOptions->InitialProps), false);
    }
    m_isJSViewAttached = true;
  }
}

void ReactRootView::ShowInstanceError() noexcept {
  if (m_xamlRootView) {
    ClearLoadingUI();
  }
}

void ReactRootView::ShowInstanceWaiting() noexcept {
  if (m_xamlRootView) {
    EnsureLoadingUI();

    // Place message into TextBlock
    std::wstring wstrMessage(L"Connecting to remote debugger");
    m_waitingTextBlock.Text(wstrMessage);
  }
}

void ReactRootView::ShowInstanceLoading() noexcept {
  if (!m_context->SettingsSnapshot().UseDeveloperSupport())
    return;

  if (m_xamlRootView) {
    EnsureLoadingUI();

    // Place message into TextBlock
    std::wstring wstrMessage(L"Loading bundle.");
    m_waitingTextBlock.Text(wstrMessage);
  }
}

void ReactRootView::EnsureFocusSafeHarbor() noexcept {
  if (!m_focusSafeHarbor) {
    m_focusSafeHarbor = xaml::Controls::ContentControl{};
    m_focusSafeHarbor.Width(0.0);
    m_focusSafeHarbor.IsTabStop(false);
    Children().InsertAt(0, m_focusSafeHarbor);

    m_focusSafeHarborLosingFocusRevoker = m_focusSafeHarbor.LosingFocus(
        winrt::auto_revoke, [this](const auto & /*sender*/, const winrt::LosingFocusEventArgs & /*args*/) {
          m_focusSafeHarbor.IsTabStop(false);
        });
  }
}

void ReactRootView::AttachBackHandlers() noexcept {
  /*
   * If we are running in a Xaml Island or some other environment where the SystemNavigationManager is unavailable,
   * we should just skip hooking up the BackButton handler. SystemNavigationManager->GetForCurrentView seems to
   * crash with XamlIslands so we can't just bail if that call fails.
   */
  if (::Microsoft::ReactNative::IsXamlIsland())
    return;

  if (winrt::Microsoft::ReactNative::implementation::QuirkSettings::GetBackHandlerKind(
          winrt::Microsoft::ReactNative::ReactPropertyBag(m_context->Properties())) !=
      winrt::Microsoft::ReactNative::BackNavigationHandlerKind::JavaScript)
    return;

  auto weakThis = this->get_weak();
  m_backRequestedRevoker = winrt::Windows::UI::Core::SystemNavigationManager::GetForCurrentView().BackRequested(
      winrt::auto_revoke,
      [weakThis](winrt::IInspectable const & /*sender*/, winrt::BackRequestedEventArgs const &args) {
        if (auto self = weakThis.get()) {
          args.Handled(self->OnBackRequested());
        }
      });

  // In addition to handling the BackRequested event, UWP suggests that we listen for other user inputs that should
  // trigger back navigation that don't fire that event:
  // https://docs.microsoft.com/en-us/windows/uwp/design/basics/navigation-history-and-backwards-navigation

  // Handle keyboard "back" button press
  xaml::Input::KeyboardAccelerator goBack{};
  goBack.Key(winrt::Windows::System::VirtualKey::GoBack);
  goBack.Invoked([weakThis](
                     xaml::Input::KeyboardAccelerator const & /*sender*/,
                     xaml::Input::KeyboardAcceleratorInvokedEventArgs const &args) {
    if (auto self = weakThis.get()) {
      args.Handled(self->OnBackRequested());
    }
  });
  KeyboardAccelerators().Append(goBack);

  // Handle Alt+Left keyboard shortcut
  xaml::Input::KeyboardAccelerator altLeft{};
  altLeft.Key(winrt::Windows::System::VirtualKey::Left);
  altLeft.Invoked([weakThis](
                      xaml::Input::KeyboardAccelerator const & /*sender*/,
                      xaml::Input::KeyboardAcceleratorInvokedEventArgs const &args) {
    if (auto self = weakThis.get()) {
      args.Handled(self->OnBackRequested());
    }
  });
  KeyboardAccelerators().Append(altLeft);
  altLeft.Modifiers(winrt::Windows::System::VirtualKeyModifiers::Menu);

  // Hide keyboard accelerator tooltips
  if (::Microsoft::ReactNative::IsRS4OrHigher()) {
    KeyboardAcceleratorPlacementMode(xaml::Input::KeyboardAcceleratorPlacementMode::Hidden);
  }
}

void ReactRootView::RemoveBackHandlers() noexcept {
  m_backRequestedRevoker.revoke();
  KeyboardAccelerators().Clear();
}

bool ReactRootView::OnBackRequested() noexcept {
  if (m_context->State() != Mso::React::ReactInstanceState::Loaded)
    return false;

  m_context->CallJSFunction("RCTDeviceEventEmitter", "emit", folly::dynamic::array("hardwareBackPress"));
  return true;
}

Mso::React::IReactViewHost *ReactRootView::ReactViewHost() noexcept {
  return m_reactViewHost.Get();
}

void ReactRootView::ReactViewHost(Mso::React::IReactViewHost *viewHost) noexcept {
  if (m_reactViewHost.Get() == viewHost) {
    return;
  }

  if (m_reactViewHost) {
    UninitRootView();
    m_reactViewHost->DetachViewInstance();
  }

  m_reactViewHost = viewHost;

  if (m_reactViewHost) {
    auto viewInstance = Mso::Make<::Microsoft::ReactNative::ReactViewInstance>(this->get_weak(), m_uiQueue);
    m_reactViewHost->AttachViewInstance(*viewInstance);
  }
}

winrt::Windows::Foundation::Size ReactRootView::MeasureOverride(
    winrt::Windows::Foundation::Size const &availableSize) const {
  winrt::Windows::Foundation::Size size{0.0f, 0.0f};

  for (xaml::UIElement child : Children()) {
    child.Measure(availableSize);
    auto desired = child.DesiredSize();
    if (desired.Height > size.Height)
      size.Height = desired.Height;
    if (desired.Width > size.Width)
      size.Width = desired.Width;
  }

  return size;
}

winrt::Windows::Foundation::Size ReactRootView::ArrangeOverride(winrt::Windows::Foundation::Size finalSize) const {
  for (xaml::UIElement child : Children()) {
    child.Arrange(winrt::Rect(0, 0, finalSize.Width, finalSize.Height));
  }
  return finalSize;
}

// Maps react-native's view of the root view to the actual UI
// react-native is unaware that there are non-RN elements within the ReactRootView
uint32_t ReactRootView::RNIndexToXamlIndex(uint32_t index) noexcept {
  // If m_focusSafeHarbor exists, it should be at index 0
  // m_xamlRootView is the next element, followed by any RN content.
#if DEBUG
  uint32_t findIndex{0};
  Assert(!m_focusSafeHarbor || Children().IndexOf(m_focusSafeHarbor, findIndex) && findIndex == 0);
  Assert(Children().IndexOf(m_xamlRootView, findIndex) && findIndex == (m_focusSafeHarbor ? 1 : 0));
#endif

  return index + (m_focusSafeHarbor ? 2 : 1);
}

void ReactRootView::AddView(uint32_t index, xaml::UIElement child) {
  Children().InsertAt(RNIndexToXamlIndex(index), child);
}

void ReactRootView::RemoveAllChildren() {
  const uint32_t numLeft = m_focusSafeHarbor ? 2 : 1;
  while (Children().Size() > numLeft)
    Children().RemoveAt(numLeft);
}

void ReactRootView::RemoveChildAt(uint32_t index) {
  Children().RemoveAt(RNIndexToXamlIndex(index));
}

bool IsCtrlShiftI(winrt::Windows::System::VirtualKey key) noexcept {
  return (
      key == winrt::Windows::System::VirtualKey::I &&
      ::Microsoft::ReactNative::IsModifiedKeyPressed(
          winrt::CoreWindow::GetForCurrentThread(), winrt::Windows::System::VirtualKey::Shift) &&
      ::Microsoft::ReactNative::IsModifiedKeyPressed(
          winrt::CoreWindow::GetForCurrentThread(), winrt::Windows::System::VirtualKey::Control));
}

void ReactRootView::SetupDevToolsShortcut() noexcept {
  if (auto xamlRoot = XamlRoot()) {
    if (std::find(m_subscribedDebuggerRoots.begin(), m_subscribedDebuggerRoots.end(), xamlRoot) ==
        m_subscribedDebuggerRoots.end()) {
      if (auto rootContent = xamlRoot.Content()) {
        m_subscribedDebuggerRoots.push_back(xamlRoot);
        rootContent.KeyDown(
            [weakThis = this->get_weak()](const auto & /*sender*/, const xaml::Input::KeyRoutedEventArgs &args) {
              if (const auto strongThis = weakThis.get()) {
                if (IsCtrlShiftI(args.Key())) {
                  ::Microsoft::ReactNative::GetSharedDevManager()->OpenDevTools(
                      winrt::to_string(strongThis->m_reactNativeHost.InstanceSettings().BundleAppId()));
                }
              };
            });
      }
    }
  }
}

} // namespace winrt::Microsoft::ReactNative::implementation
