package com.dava.engine;

import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.webkit.WebChromeClient;
import android.webkit.JsResult;

class DavaWebViewClient extends WebViewClient
{
    private DavaWebView davaWebView;

    public DavaWebViewClient(DavaWebView webview)
    {
        davaWebView = webview;
    }

    @Override
    public void onPageFinished(WebView view, String url)
    {
        davaWebView.onPageFinished(url);
    }

    @Override
    public boolean shouldOverrideUrlLoading(WebView view, String url)
    {
        return davaWebView.shouldOverrideUrlLoading(url);
    }

    @Override
    public void onReceivedError(WebView view, int errorCode, String description, String failingUrl)
    {
        davaWebView.onReceivedError(errorCode, description, failingUrl);
    }
}

class DavaWebViewChromeClient extends WebChromeClient
{
    private DavaWebView davaWebView;

    public DavaWebViewChromeClient(DavaWebView webview)
    {
        davaWebView = webview;
    }

    @Override
    public boolean onJsAlert(WebView view, String url, String message, JsResult result)
    {
        davaWebView.onJsAlert(url, message);
        result.confirm();
        return true;
    }
}
