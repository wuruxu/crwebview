// color value : https://developer.mozilla.org/zh-CN/docs/Web/CSS/color_value
console.log("javascript run_at document_end, set background skyblue");
document.body.style.background = 'skyblue';
console.log("message ID: " + chrome.runtime.id);
console.log("Manifest: " + JSON.stringify(chrome.runtime.getManifest()));
chrome.runtime.sendMessage('get-user-data', (response) => {
  console.log('content-script.js received user data', response);
});
