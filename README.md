# miniblink-demo

本项目为 [miniblink](https://github.com/weolar/miniblink49) 的使用示例demo，方便大家学习参考。

API的使用参考：<https://mb-simple-guide.vercel.app/api/all.html> (须借助梯子)

## 如何使用

建议直接使用官方编译的，替换官方发布的[release](https://github.com/weolar/miniblink49/releases)包里面的`mb.h`和`dll`文件即可。

具体例子可以参考本demo代码及运行效果，如果想要自行编译，参见后面章节。

## 编译miniblink

本次编译不具有通用性，建议官方先根据先前反馈的问题先修改一版，后面我再修改下编译说明。此次发现的问题有：

- 含有绝对路径的头文件、lib文件包含；
- 含有不正确的头文件包含，可能文件路径已经发生变更了，本地可能存在多个拷贝；
- 有不安全的字符串api使用，需要使用_s版本的；
- UrlUtil.h需要修改替换，已发送；
- 有找不到的lib文件，例如FFmpeg.dll.lib，我先传到本git项目下保存了；
- 由于需要兼容xp，使用的sdk版本较低（7，目前是10），需要额外安装D3D，需要说明下，另外应允许其他使用者使用Windows10编译，开发者可以设置一个编译开关的头文件，用来自动判断；建议默认编译选项使用的SDK应该是最新的10，而不是7，使用7的自行打开或切换编译开关即可；
- 建议在代码里面做lib库的链接，避免使用编译选项，修改起来很麻烦；
- 建议在代码里面增加一个统一的编译开关头文件，方便修改及编译；
- 工程中存在v8多个版本，如果编译开关文件存在，也可以在通过编译开关来自由选择链接的v8版本；

在手动解决以上的一些问题后

- 修改libcurl的编译属性，然后增加：

```
_WINSOCK_DEPRECATED_NO_WARNINGS
_CRT_SECURE_NO_WARNINGS
```

消除：`inet_addr / GetVersionExW` 的编译错误。
如果还有`GetVersionEx`的编译错误，找到这段代码，注释掉即可。

编译，最后出现几个链接错误：

```
1>libcurl.lib(ares_init.obj) : error LNK2019: 无法解析的外部符号 __imp__ares_inet_ntop，函数 _get_DNS_AdaptersAddresses 中引用了该符号
1>libcurl.lib(ares_gethostbyname.obj) : error LNK2019: 无法解析的外部符号 __imp__ares_search，函数 _host_callback 中引用了该符号
1>libcurl.lib(ares_query.obj) : error LNK2019: 无法解析的外部符号 __imp__ares_send，函数 _ares_query 中引用了该符号
1>libcurl.lib(ares_query.obj) : error LNK2019: 无法解析的外部符号 __imp__ares_create_query，函数 _ares_query 中引用了该符号
1>libcurl.lib(ares_query.obj) : error LNK2019: 无法解析的外部符号 __imp__ares_free_string，函数 _ares_query 中引用了该符号
1>libcurl.lib(ares_parse_naptr_reply.obj) : error LNK2019: 无法解析的外部符号 __imp__ares_expand_string，函数 _ares_parse_naptr_reply 中引用了该符号
fatal error LNK1120: 6 个无法解析的外部命令
```

未完待续。。。

## 注册Native函数并实现同步调用

主要思路：使用消息+json+Promise和await实现伪同步。

先参考下使用`WebView2`实现的方法，调用`AddScriptToExecuteOnDocumentCreated`注入如下代码。

```js
let __cpp_call_id = 0;              // 全局唯一请求 ID
const __cpp_callbacks = new Map();  // 存储 Promise 的 resolve

// JS 发起调用 C++
function callCpp(method, args) {
    return new Promise((resolve) => {
     const id = ++__cpp_call_id;
     __cpp_callbacks.set(id, resolve);

     // 向 C++ 发送消息
     const msg = { id, method, args };
     chrome.webview.postMessage(JSON.stringify(msg));
    });
}

// 接收来自 C++ 的返回值
window.chrome.webview.addEventListener('message', (event) => {
    try {
     const data = JSON.parse(event.data);
     const { id, result } = data;
     if (__cpp_callbacks.has(id)) {
      const resolve = __cpp_callbacks.get(id);
      resolve(result); // 调用
      __cpp_callbacks.delete(id);
     }
    } catch (err) {
     alert('接收 C++ 消息失败:', err);
    }
});
```

这样我们就注册好了一个万能的函数：`callCpp` ，调用的时候，可以这么调用：

```js
callCpp('getUserInfo', { userId: 123 }).then((res) => { alert("成功：" + JSON.stringify(res)); });
```

或：

```js
return await callCpp('getUserInfo', { userId: 123 });
```

但是为了使用方便，我们可以通过这个万能函数进行扩展，如下我们封装了两个示例函数：

```js
// 封装 addInt(a, b) -> 返回 int
async function addInt(a, b) {
    return await callCpp('addInt', { a, b });
}

// 封装 addStr(a, b) -> 返回 string
async function addStr(a, b) {
    return await callCpp('addStr', { a, b });
}
```

绑定 JS 调用 C++ 的处理函数：

```c
void CMainWnd::registerNativeCall() {
 m_webview->add_WebMessageReceived(
  Callback<ICoreWebView2WebMessageReceivedEventHandler>(
   [](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* pArgs) -> HRESULT {
  wil::unique_cotaskmem_string message;
  pArgs->TryGetWebMessageAsString(&message);

  // 解析 JSON
  std::wstring jsonW(message.get());
  std::string json = Star::StrUnit::ws2utf8s(jsonW);

  // 你可以用任意 JSON 库，如 nlohmann/json
  nlohmann::json j = nlohmann::json::parse(json);
  int id = j["id"];
  std::string method = j["method"];
  nlohmann::json args = j["args"];

  // 模拟返回值
  nlohmann::json result;
  result["id"] = id;
  result["success"] = true;
  if (method == "getUserInfo") {
   int userId = args["userId"];
   result["result"] = {
    { "userId", userId},
    { "name", "jack" },
    { "age", 25 }
   };
  } else if (method == "addInt") {
   int sum = args["a"].get<int>() + args["b"].get<int>();
   result["result"] = sum;
  } else if (method == "addStr") {
   std::string s = args["a"].get<std::string>() + args["b"].get<std::string>();
   result["result"] = s;
  } else {
   result["result"] = method + " not implemented";
  }

  // 返回给 JS
  std::string resultStr;
  try {
   resultStr = result.dump();
  } catch (const std::exception& e) {
   resultStr = e.what();
  }
  std::wstring resultW = Star::StrUnit::utf8s2ws(resultStr);
  sender->PostWebMessageAsString(resultW.c_str());

  return S_OK;
 }
 ).Get(), nullptr);
}
```

然后在html里我们做一个测试代码：

```js
<script>
async function test() {
    //callCpp('getUserInfo', { userId: 123 }).then((res) => { alert("成功：" + JSON.stringify(res)); });
    return await callCpp('getUserInfo', { userId: 123 });
}

async function testNoFunction() {
 return await callCpp('testNoFunction', {});
}

// 把同步顺序逻辑放到一个 async 函数中
(async () => {
    try {
        alert('同步调用执行顺序 begin');
  
        const res = await test();
        alert("成功：" + JSON.stringify(res));
  
  const x1 = await addInt(3, 5);
  alert('addInt -> ' + x1);

  const x2 = await addStr('你好', 'world');
  alert('addStr -> ' + x2);
  
  const x3 = await testNoFunction();
  alert('testNoFunction -> ' + x3);
  
        alert('同步调用执行顺序 end');
    } catch (e) {
        alert(e);
    }
})();
</script>
```

均是按照顺序弹框的，且结果符合预期。

我们再看实用mb如何实现，mb有类似的接口：`window.mbQuery`，使用方法如下：

```js
function onNativeResponse(customMsg, response) {
    alert('mbQuery:' + response);
};

try {
    console.log('hello js');
    var ret = window.mbQuery(123456, "I am in js context", onNativeResponse);
    alert(ret);
} catch(e){
    alert(e);
}
```

但是这种效果是异步的，且只有两个参数。要想实现不定个数参数的自定义函数，可以将拓展这个字符串，使用json格式来去构造。然后再结合Promise和await实现伪同步。

可以在onJsQueryCallback里调用mbResponseQuery设置返回结果：

```c
void MB_CALL_TYPE onJsQueryCallback(mbWebView webView, void* param, mbJsExecState es, int64_t queryId, int customMsg, const utf8* request) {
    printf("onJsQueryCallback\n");
    if (customMsg== 123456) {
        mbResponseQuery(webView, queryId, customMsg, "I am response");
    }
}
```

但是很遗憾，这个结果并不能同步返回回去。

由于对mb不太了解，这里先总结下综合的疑问和建议，反馈给作者，看看下一步是否可以优化下：

- mbResponseQuery 是否可以返回结果给到js，同步或异步的均可。
- 在mb下是否支持类似的`window.chrome.webview.addEventListener('message')`添加消息监听，是否支持类似`chrome.webview.postMessage`发送消息。
- 实现类似AddScriptToExecuteOnDocumentCreated的接口用来注入js代码。
- 作者可以参考上述思路，优化实现更优雅的方法。
