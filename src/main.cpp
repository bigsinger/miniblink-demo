#include <iostream>
#include "mb.h"


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
}

// 页面DOM发出ready事件时触发此回调
void MB_CALL_TYPE onDocumentReadyCallback(mbWebView webView, void* param, mbWebFrameHandle frameId) {
	printf("onDocumentReadyCallback\n");

	// 这里可以获取页面的HTML内容
    mbStringPtr ptr = mbGetSourceSync(mbView);
	auto len = mbGetStringLen(ptr);
    auto html = mbGetString(ptr);
	printf("Document ready, HTML length: %zu %d\n", len, strlen(html));
    mbDeleteString(ptr);

	// 执行一段JS测试代码
    mbRunJs(mbView, mbWebFrameGetMainFrame(mbView), 
        R"(
        try {
            console.log('hello js');
            console.log2('');
        } catch(e){
            alert(e);
        }
        )",
        true, onRunJsCallback, nullptr, nullptr);
}

void MB_CALL_TYPE onJsQueryCallback(mbWebView webView, void* param, mbJsExecState es, int64_t queryId, int customMsg, const utf8* request) {
    printf("onJsQueryCallback\n");
}

BOOL MB_CALL_TYPE onLoadUrlBegin(mbWebView webView, void* param, const char* url, mbNetJob job) {
    printf("onLoadUrlBegin\n");
    return false;
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
    
    // 注册js通知native的回调。配合mbResponseQuery接口，用于实现js调用C++
    mbOnJsQuery(mbView, onJsQueryCallback, NULL);
    
    mbOnLoadUrlBegin(mbView, onLoadUrlBegin, NULL);


	// 居中显示窗口
    mbShowWindow(mbView, TRUE);
    mbMoveToCenter(mbView);
    
    // 加载测试URL
    mbLoadURL(mbView, TestUrl);

    mbRunMessageLoop();

    mbUninit();
    return 0;
}