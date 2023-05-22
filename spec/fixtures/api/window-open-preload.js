const { ipcRenderer, webFrame } = require('electron');

setImmediate(function () {
  if (window.location.toString() === 'bar://page/') {
    const windowOpenerIsNull = window.opener == null;
    ipcRenderer.send('answer', {
      nativeWindowOpen: webFrame.getWebPreference('nativeWindowOpen'),
      nodeIntegration: webFrame.getWebPreference('nodeIntegration'),
      sandbox: webFrame.getWebPreference('sandbox'),
      typeofProcess: typeof global.process,
      windowOpenerIsNull
    });
    window.close();
  }
});
