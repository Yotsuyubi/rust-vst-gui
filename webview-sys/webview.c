#include "webview.h"
#include <stdio.h>
#include <string.h>

#define WKUserScriptInjectionTimeAtDocumentStart 0

#define _cls(s) ((id)objc_getClass(s))
#define _sel(s) (sel_registerName(s))
#define _str(s) (objc_msgSend((id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), s))

void eval_webview(id webview, const char *js) {
  objc_msgSend(webview, _sel("evaluateJavaScript:completionHandler:"), _str(js), nil);
}

void prompt_listener(id self, SEL cmd, id webView, id message, id defaultText, id frame, void (^completionHandler)(id)) {
  // TODO null check strings (message)
  // TODO handle cases where we actually need a prompt window
  const char *body = (const char *)objc_msgSend(message, _sel("UTF8String"));
  callback cb = (callback)objc_getAssociatedObject(self, "callback");
  const void *data = (const void *)objc_getAssociatedObject(self, "data");
  const char *retval = cb(data, body);
  completionHandler(_str(retval));
}

id create_webview(const void *parent, int width, int height, const char *html, const void* data, const void* callback) {
  id config = objc_msgSend(_cls("WKWebViewConfiguration"), _sel("new"));
  id manager = objc_msgSend(config, _sel("userContentController"));

  // Web View
  id webview = objc_msgSend(_cls("WKWebView"), _sel("alloc"));

  objc_msgSend(webview, _sel("initWithFrame:configuration:"), CGRectMake(0, 0, width, height), config);
  objc_msgSend(webview, _sel("loadHTMLString:baseURL:"), _str(html), nil);

  objc_msgSend((id)parent, _sel("addSubview:"), webview);

  // TODO make this disabled by default
  objc_msgSend(objc_msgSend(config, _sel("preferences")), _sel("setValue:forKey:"), objc_msgSend(_cls("NSNumber"), _sel("numberWithBool:"), 1), _str("developerExtrasEnabled"));

  // Delegate
  static Class __WKUIDelegate;
  if(__WKUIDelegate == NULL) {
    __WKUIDelegate = objc_allocateClassPair((Class) _cls("NSObject"), "__WKUIDelegate", 0);
    class_addProtocol(__WKUIDelegate, objc_getProtocol("WKUIDelegate"));
    class_addMethod(__WKUIDelegate, _sel("webView:runJavaScriptTextInputPanelWithPrompt:defaultText:initiatedByFrame:completionHandler:"), (IMP)(prompt_listener), "v@:@@@@?");
    // I don't know why bug fixed on comment below out.
    // objc_registerClassPair(__WKUIDelegate);
  }

  id uiDel = objc_msgSend((id)__WKUIDelegate, _sel("new"));
  objc_setAssociatedObject(uiDel, "callback", callback, OBJC_ASSOCIATION_ASSIGN);
  objc_setAssociatedObject(uiDel, "data", data, OBJC_ASSOCIATION_ASSIGN);
  objc_msgSend(webview, _sel("setUIDelegate:"), uiDel);

  // inject invoke function that uses window.prompt hack (https://stackoverflow.com/a/39728672)
  char *js = "external = {invoke:(s) => {console.log('sending:', s); return prompt(s);}}";
  objc_msgSend(manager, _sel("addUserScript:"),
      objc_msgSend(objc_msgSend(_cls("WKUserScript"), _sel("alloc")),
        _sel("initWithSource:injectionTime:forMainFrameOnly:"),
        _str(js),
        WKUserScriptInjectionTimeAtDocumentStart, 1));

  return webview;
}

void destroy_webview(id webview) {
  objc_msgSend(webview, _sel("release"));
}
