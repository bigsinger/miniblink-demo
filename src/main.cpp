#include <vector>
#include <iostream>
#include "mb.h"
#include "Utils.h"


/////////////////////////////////////////
/// <summary>
/// 宏定义 常量定义
/// </summary>
/////////////////////////////////////////

// 希望使用的mb版本
#define UsingMbVersion 132                

/* 辅助宏：把宏值转成宽字符串 */
#define WSTR_(x)  L## #x
#define WSTR(x)   WSTR_(x)

#ifdef _WIN64
#  define MbDllName  L"mb" WSTR(UsingMbVersion) L"_x64.dll"
#else
#  define MbDllName  L"mb" WSTR(UsingMbVersion) L"_x32.dll"
#endif


const char TestUrl[] = "https://miniblink.net/views/doc/index.html";
/////////////////////////////////////////



/////////////////////////////////////////
/// <summary>
/// 保持mb创建的webview
/// </summary>
/////////////////////////////////////////
mbWebView mbView = NULL;
/////////////////////////////////////////


/////////////////////////////////////////
/// <summary>
/// 一些回调函数
/// </summary>
/////////////////////////////////////////

// 关闭回调。这里可以添加一些清理工作。
BOOL MB_CALL_TYPE onCloseCallback(mbWebView webView, void* param, void* unuse) {
    printf("onCloseCallback\n");
	mbExitMessageLoop();    // ExitProcess
    return TRUE;
}

// 执行JS的回调函数
void MB_CALL_TYPE onRunJsCallback(mbWebView webView, void* param, mbJsExecState es, mbJsValue v) {
    printf("onRunJsCallback\n");
    const utf8* str = mbJsToString(es, v);
	printf("%s\n", str);
}

// 页面DOM发出ready事件时触发此回调
void MB_CALL_TYPE onDocumentReadyCallback(mbWebView webView, void* param, mbWebFrameHandle frameId) {
	printf("onDocumentReadyCallback\n");

	// 这里可以获取页面的HTML内容
    if (mbGetSourceSync) {
        mbStringPtr ptr = mbGetSourceSync(mbView);
        auto len = mbGetStringLen(ptr);
        auto html = mbGetString(ptr);
        printf("Document ready, HTML length: %zu %zu\n", len, strlen(html));
        mbDeleteString(ptr);
    } else {
        printf("mbGetSourceSync is null\n");
    }

	// 执行一段JS测试代码
    mbRunJs(mbView, mbWebFrameGetMainFrame(mbView), 
        R"(
        let __cpp_call_id = 0;				// 全局唯一请求 ID
		const __cpp_callbacks = new Map();	// 存储 Promise 的 resolve

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
					resolve(result);	// 调用
					__cpp_callbacks.delete(id);
				}
			} catch (err) {
				alert('接收 C++ 消息失败:', err);
			}
		});


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
        )",
        true, onRunJsCallback, nullptr, nullptr);
}

void MB_CALL_TYPE onLoadingFinish(mbWebView webView, void* param, mbWebFrameHandle frameId, const utf8* url, mbLoadingResult result, const utf8* failedReason) {
    //if(result == MB_LOADING_SUCCEEDED)
    //::mbNetGetFavicon(webView, HandleFaviconReceived, param);
    printf("onLoadingFinish\n");
}

void MB_CALL_TYPE onJsQueryCallback(mbWebView webView, void* param, mbJsExecState es, int64_t queryId, int customMsg, const utf8* request) {
    printf("onJsQueryCallback\n");
    if (customMsg== 123456) {
        mbResponseQuery(webView, queryId, customMsg, "I am response");
    }
}

BOOL MB_CALL_TYPE onLoadUrlBegin(mbWebView webView, void* param, const char* url, mbNetJob job) {
    printf("onLoadUrlBegin\n");
    return false;
}

void MB_CALL_TYPE onLoadUrlEnd
(mbWebView webView, void* param, const char* url, void* job, void* buf, int len) {
    printf("onLoadUrlEnd\n");
}

void MB_CALL_TYPE onUrlChanged(mbWebView webView, void* param, const utf8* url, BOOL canGoBack, BOOL canGoForward) {
    printf("onTitleChanged, canGoBack: %d, canGoForward: %d, url: %s\n", canGoBack, canGoForward, url);
}

void MB_CALL_TYPE onTitleChanged(mbWebView webView, void* param, const utf8* title) {
    std::wstring titleString;
    if (title) { titleString = utf8ToUtf16(title); }
    wprintf(L"onTitleChanged: %s\n", titleString.c_str());
}

/////////////////////////////////////////


int main() {
	wprintf(L"Miniblink Demo, Load: %s\n", MbDllName);
    mbSetMbMainDllPath(MbDllName);

    mbSettings* settings = new mbSettings();
    memset(settings, 0, sizeof(mbSettings));
    mbInit(settings);

    // 创建一个带真实窗口的mbWebView,若使用内嵌窗口可使用WKE_WINDOW_TYPE_CONTROL
    mbView = mbCreateWebWindow(MB_WINDOW_TYPE_POPUP, nullptr, 0, 0, 800, 600);
    
    // 注册关闭回调
    mbOnClose(mbView, onCloseCallback, NULL);
    
	// 页面DOM发出ready事件时触发此回调
    mbOnDocumentReady(mbView, onDocumentReadyCallback, NULL);
    
    mbOnLoadingFinish(mbView, onLoadingFinish, NULL);

    // 注册js通知native的回调。配合mbResponseQuery接口，用于实现js调用C++
    mbOnJsQuery(mbView, onJsQueryCallback, NULL);
    
    mbOnLoadUrlBegin(mbView, onLoadUrlBegin, NULL);
    mbOnLoadUrlEnd(mbView, onLoadUrlEnd, NULL);

    mbOnURLChanged(mbView, onUrlChanged, NULL); 

    mbOnTitleChanged(mbView, onTitleChanged, NULL);

    // 不打开新窗口
	mbSetNavigationToNewWindowEnable(mbView, FALSE); 


	// 居中显示窗口
    mbResize(mbView, 800, 600);
    mbShowWindow(mbView, TRUE);
    mbMoveToCenter(mbView);
    
    // 加载测试URL
    mbLoadURL(mbView, TestUrl);

    mbRunMessageLoop();

    mbUninit();
    return 0;
}