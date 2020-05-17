// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"

#include <ReactPropertyBag.h>
#include <winrt/Microsoft.ReactNative.h>
#include <winrt/Windows.Foundation.h>
#include <string>
#include <eventWaitHandle/eventWaitHandle.h>

using namespace winrt;
using namespace Microsoft::ReactNative;
using namespace Windows::Foundation;
using namespace std::string_literals;

namespace ReactNativeIntegrationTests {

TEST_CLASS (ReactNotificationServiceTests) {
  TEST_METHOD(Notification_Subscribe) {
    auto fooName = ReactPropertyBagHelper::GetName(nullptr, L"Foo");
    IReactNotificationService rnf{ReactNotificationServiceHelper::CreateNotificationService()};
    bool isCalled{false};
    rnf.Subscribe(nullptr, fooName, [&](IInspectable const &sender, IReactNotificationData const &data) noexcept {
      isCalled = true;
      TestCheck(!sender);
      TestCheck(!data.Data());
      TestCheck(!data.Subscription().Dispatcher());
      TestCheckEqual(fooName, data.Subscription().NotificationName());
      TestCheck(data.Subscription().IsSubscribed());
    });
    rnf.SendNotification(fooName, nullptr, nullptr);
    TestCheck(isCalled);
  }

  TEST_METHOD(Notification_Unsubscribe) {
    IReactNotificationService rnf{ReactNotificationServiceHelper::CreateNotificationService()};
    auto fooName = ReactPropertyBagHelper::GetName(nullptr, L"Foo");
    bool isCalled{false};
    auto subscription = rnf.Subscribe(
        nullptr, fooName, [&](IInspectable const & /*sender*/, IReactNotificationData const & /*data*/) noexcept {
          isCalled = true;
        });
    rnf.SendNotification(fooName, nullptr, nullptr);
    TestCheck(isCalled);

    subscription.Unsubscribe();
    TestCheck(!subscription.IsSubscribed());

    isCalled = false;
    rnf.SendNotification(fooName, nullptr, nullptr);
    TestCheck(!isCalled);
  }

  TEST_METHOD(Notification_UnsubscribeInHandler) {
    IReactNotificationService rnf{ReactNotificationServiceHelper::CreateNotificationService()};
    auto fooName = ReactPropertyBagHelper::GetName(nullptr, L"Foo");
    bool isCalled{false};
    auto subscription = rnf.Subscribe(
        nullptr, fooName, [&](IInspectable const & /*sender*/, IReactNotificationData const &data) noexcept {
          isCalled = true;
          data.Subscription().Unsubscribe();
        });
    rnf.SendNotification(fooName, nullptr, nullptr);
    TestCheck(isCalled);

    isCalled = false;
    rnf.SendNotification(fooName, nullptr, nullptr);
    TestCheck(!isCalled);
  }

  TEST_METHOD(Notification_SenderAndData) {
    IReactNotificationService rnf{ReactNotificationServiceHelper::CreateNotificationService()};
    auto fooName = ReactPropertyBagHelper::GetName(nullptr, L"Foo");
    auto mySender = box_value(L"Hello");
    auto myData = box_value(42);
    bool isCalled{false};
    rnf.Subscribe(nullptr, fooName, [&](IInspectable const &sender, IReactNotificationData const &data) noexcept {
      isCalled = true;
      TestCheckEqual(mySender, sender);
      TestCheckEqual(myData, data.Data());
    });
    rnf.SendNotification(fooName, mySender, myData);
    TestCheck(isCalled);
  }

  TEST_METHOD(Notification_InQueue) {
    IReactNotificationService rnf{ReactNotificationServiceHelper::CreateNotificationService()};
    Mso::ManualResetEvent finishedEvent;
    auto fooName = ReactPropertyBagHelper::GetName(nullptr, L"Foo");
    IReactDispatcher dispatcher = ReactDispatcherHelper::CreateSerialDispatcher();
    bool isCalled{false};
    rnf.Subscribe(dispatcher, fooName, [&](IInspectable const &/*sender*/, IReactNotificationData const &data) noexcept {
      TestCheckEqual(dispatcher, data.Subscription().Dispatcher());
      TestCheck(dispatcher.HasThreadAccess());
      isCalled = true;
      finishedEvent.Set();
    });
    rnf.SendNotification(fooName, nullptr, nullptr);
    finishedEvent.Wait();
    TestCheck(isCalled);
  }
};

} // namespace ReactNativeIntegrationTests
