#include "pch.h"

#include <winrt/Microsoft.ReactNative.h>
#include <winrt/Windows.Foundation.h>

using namespace winrt;
using namespace Microsoft::ReactNative;

namespace ReactNativeIntegrationTests {

TEST_CLASS (PropertyBagTests) {
  TEST_METHOD(StoreNamespace) {
    // Return the same namespace object for the same string.
    auto ns1 = ReactPropertyBag::GetNamespace(L"Foo");
    auto ns2 = ReactPropertyBag::GetNamespace(L"Foo");
    TestCheck(ns1);
    TestCheck(ns2);
    TestCheck(ns1 == ns2);
  }

  TEST_METHOD(WeakNamespace) {
    // Property bag keeps a weak reference to namespaces.
    weak_ref<IReactPropertyNamespace> nsWeak;
    {
      auto ns = ReactPropertyBag::GetNamespace(L"Foo");
      TestCheck(ns);
      nsWeak = ns;
      TestCheck(nsWeak.get());
    }
    TestCheck(!nsWeak.get());
  }

  TEST_METHOD(GlobalNamespace) {
    // Global namespace is the same as the empty string
    auto ns1 = ReactPropertyBag::GlobalNamespace();
    auto ns2 = ReactPropertyBag::GetNamespace(L"");
    TestCheck(ns1);
    TestCheck(ns2);
    TestCheck(ns1 == ns2);
  }

  TEST_METHOD(GlobalNamespaceNull) {
    // Global namespace is the same as null string
    auto ns1 = ReactPropertyBag::GlobalNamespace();
    hstring nullStr{nullptr, take_ownership_from_abi};
    auto ns2 = ReactPropertyBag::GetNamespace(nullStr);
    TestCheck(ns1);
    TestCheck(ns2);
    TestCheck(ns1 == ns2);
  }

  TEST_METHOD(WeakGlobalNamespace) {
    // Property bag keeps a weak reference to the Global namespace.
    weak_ref<IReactPropertyNamespace> globalWeak;
    {
      auto global = ReactPropertyBag::GlobalNamespace();
      TestCheck(global);
      globalWeak = global;
      TestCheck(globalWeak.get());
    }
    TestCheck(!globalWeak.get());
  }

  TEST_METHOD(StoreName) {
    // Return the same namespace object for the same string.
    auto ns1 = ReactPropertyBag::GetNamespace(L"Foo");
    auto n11 = ReactPropertyBag::GetName(ns1, L"FooName");
    auto n12 = ReactPropertyBag::GetName(ns1, L"FooName");
    TestCheck(n11);
    TestCheck(n12);
    TestCheck(n11 == n12);
  }

  TEST_METHOD(StoreNameDifferentNamespace) {
    // Return different name objects for the same string in different namespaces.
    auto ns1 = ReactPropertyBag::GetNamespace(L"Foo1");
    auto ns2 = ReactPropertyBag::GetNamespace(L"Foo2");
    auto n11 = ReactPropertyBag::GetName(ns1, L"FooName");
    auto n21 = ReactPropertyBag::GetName(ns2, L"FooName");
    TestCheck(n11);
    TestCheck(n21);
    TestCheck(n11 != n21);
  }

  TEST_METHOD(WeakName) {
    // Property bag keeps a weak reference to namespaces.
    weak_ref<IReactPropertyName> nWeak;
    {
      auto ns = ReactPropertyBag::GetNamespace(L"Foo");
      auto n = ReactPropertyBag::GetName(ns, L"Foo");
      TestCheck(ns);
      TestCheck(n);
      nWeak = n;
      TestCheck(nWeak.get());
    }
    TestCheck(!nWeak.get());
  }

  TEST_METHOD(GlobalNamespaceName) {
    // null namespace is the same as global.
    auto n1 = ReactPropertyBag::GetName(nullptr, L"Foo");
    auto n2 = ReactPropertyBag::GetName(ReactPropertyBag::GlobalNamespace(), L"Foo");
    auto n3 = ReactPropertyBag::GetName(ReactPropertyBag::GetNamespace(L""), L"Foo");
    TestCheck(n1 == n2);
    TestCheck(n1 == n3);
  }

  TEST_METHOD(GetProperty_DoesNotExist) {
    auto fooName = ReactPropertyBag::GetName(nullptr, L"Foo");
    ReactPropertyBag pb;
    auto value = pb.GetProperty(fooName);
    TestCheck(!value);
  }

  TEST_METHOD(GetProperty_Int) {
    auto fooName = ReactPropertyBag::GetName(nullptr, L"Foo");
    ReactPropertyBag pb;
    pb.SetProperty(fooName, box_value(5));
    auto value = pb.GetProperty(fooName);
    TestCheck(value);
    TestCheckEqual(5, unbox_value<int>(value));
  }

  TEST_METHOD(GetOrCreateProperty_Int) {
    auto fooName = ReactPropertyBag::GetName(nullptr, L"Foo");
    ReactPropertyBag pb;
    auto value = pb.GetOrCreateProperty(fooName, []() { return box_value(5); });
    TestCheck(value);
    TestCheckEqual(5, unbox_value<int>(value));
  }

  TEST_METHOD(SetProperty_Int) {
    auto fooName = ReactPropertyBag::GetName(nullptr, L"Foo");
    ReactPropertyBag pb;

    auto value1 = pb.GetProperty(fooName);
    TestCheck(!value1);

    pb.SetProperty(fooName, box_value(5));
    auto value2 = pb.GetProperty(fooName);
    TestCheck(value2);
    TestCheckEqual(5, unbox_value<int>(value2));

    pb.SetProperty(fooName, box_value(10));
    auto value3 = pb.GetProperty(fooName);
    TestCheck(value3);
    TestCheckEqual(10, unbox_value<int>(value3));

    pb.SetProperty(fooName, nullptr);
    auto value4 = pb.GetProperty(fooName);
    TestCheck(!value4);
  }

  TEST_METHOD(TwoProperties) {
    auto fooName = ReactPropertyBag::GetName(nullptr, L"Foo");
    auto barName = ReactPropertyBag::GetName(nullptr, L"Bar");
    ReactPropertyBag pb;

    pb.SetProperty(fooName, box_value(5));
    pb.SetProperty(barName, box_value(L"Hello"));

    auto value1 = pb.GetProperty(fooName);
    TestCheck(value1);
    TestCheckEqual(5, unbox_value<int>(value1));

    auto value2 = pb.GetProperty(barName);
    TestCheck(value2);
    TestCheckEqual(L"Hello", unbox_value<hstring>(value2));
  }

  TEST_METHOD(RemoveProperty_Int) {
    auto fooName = ReactPropertyBag::GetName(nullptr, L"Foo");
    ReactPropertyBag pb;

    pb.SetProperty(fooName, box_value(5));
    auto value1 = pb.GetProperty(fooName);
    TestCheck(value1);
    TestCheckEqual(5, unbox_value<int>(value1));

    pb.RemoveProperty(fooName);
    auto value2 = pb.GetProperty(fooName);
    TestCheck(!value2);
  }
};

} // namespace ReactNativeIntegrationTests
