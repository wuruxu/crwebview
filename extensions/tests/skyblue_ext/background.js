const user = {
  username: 'android-demo-user'
};

console.log(" *** This is a demo console message from background.js");
//var keys = Object.getOwnPropertyNames(self);
//console.log("keys.length " + keys.length);
//for(var i = 0; i < keys.length; ++i ) {
//  console.log("keys[" + i + "]" + " = " + keys[i]);
//}
console.log("Manifest: " + JSON.stringify(chrome.runtime.getManifest()));
chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  console.log(" ===== onMessage ====== :  " + message);
  if (message === 'get-user-data') {
    sendResponse(user);
    console.log("background.js send Response");
  }
});
