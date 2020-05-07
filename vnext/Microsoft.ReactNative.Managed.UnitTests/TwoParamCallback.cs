// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace Microsoft.ReactNative.Managed.UnitTests
{
  public struct TwoParams
  {
    public bool BoolValue;
    public string StringValue;
  }

  static class TwoParamsSerialization
  {
    // Writing TwoParams value as two arguments to the IJSValueWriter.
    public static void WriteValue(this IJSValueWriter writer, TwoParams value)
    {
      writer.WriteValue(value.BoolValue);
      writer.WriteValue(value.StringValue);
    }
  }

  [ReactModule]
  class TwoParamCallbackModule
  {
    [ReactMethod]
    public async void PrintLabel(int x, int y, Action<TwoParams> callback)
    {
      bool isGoodOne = await Task.Run(() => x + y > 10);
      if (isGoodOne)
      {
        callback(new TwoParams { BoolValue = true, StringValue = "The value is good" });
      }
      else
      {
        callback(new TwoParams { BoolValue = false, StringValue = "The value is not so bad" });
      }
    }
  }

  [TestClass]
  public class TwoParamCallbackTest
  {
    private ReactModuleBuilderMock m_moduleBuilderMock;
    private ReactModuleInfo m_moduleInfo;
    private TwoParamCallbackModule m_module;

    [TestInitialize]
    public void Initialize()
    {
      // This line is only needed for 0.63 and above.
      JSValueWriterGenerator.RegisterAssembly(typeof(Point).Assembly);

      m_moduleBuilderMock = new ReactModuleBuilderMock();
      m_moduleInfo = new ReactModuleInfo(typeof(TwoParamCallbackModule));
      m_module = m_moduleBuilderMock.CreateModule<TwoParamCallbackModule>(m_moduleInfo);
    }

    [TestMethod]
    public void TestPrintLabel()
    {
      m_moduleBuilderMock.Call12(nameof(TwoParamCallbackModule.PrintLabel), 3, 8, (bool b, string s) => {
        Assert.AreEqual(true, b);
        Assert.AreEqual("The value is good", s);
      }).Wait();
      Assert.IsTrue(m_moduleBuilderMock.IsResolveCallbackCalled);
    }
  }
}
