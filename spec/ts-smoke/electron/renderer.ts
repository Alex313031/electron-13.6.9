
import {
  desktopCapturer,
  ipcRenderer,
  remote,
  webFrame,
  clipboard,
  crashReporter,
  screen,
  shell
} from 'electron'

import * as fs from 'fs'

// In renderer process (web page).
// https://github.com/electron/electron/blob/master/docs/api/ipc-renderer.md
console.log(ipcRenderer.sendSync('synchronous-message', 'ping')) // prints "pong"

ipcRenderer.on('asynchronous-reply', (event, arg: any) => {
  console.log(arg) // prints "pong"
  event.sender.send('another-message', 'Hello World!')
})

ipcRenderer.send('asynchronous-message', 'ping')

// remote
// https://github.com/electron/electron/blob/master/docs/api/remote.md

const BrowserWindow = remote.BrowserWindow
const win = new BrowserWindow({ width: 800, height: 600 })
win.loadURL('https://github.com')

remote.getCurrentWindow().on('close', () => {
  // blabla...
})

remote.getCurrentWindow().capturePage().then(image => {
  fs.writeFile('/tmp/screenshot.png', image.toBitmap(), err => {
    console.log(err)
  })
})

remote.getCurrentWebContents().print()

remote.getCurrentWindow().capturePage().then(image => {
  remote.require('fs').writeFile('/tmp/screenshot.png', image.toBitmap(), (err: Error) => {
    console.log(err)
  })
})

// web-frame
// https://github.com/electron/electron/blob/master/docs/api/web-frame.md

webFrame.setZoomFactor(2)
console.log(webFrame.getZoomFactor())

webFrame.setZoomLevel(200)
console.log(webFrame.getZoomLevel())

webFrame.setVisualZoomLevelLimits(50, 200)

webFrame.setSpellCheckProvider('en-US', {
  spellCheck (words, callback) {
    setTimeout(() => {
      const spellchecker = require('spellchecker')
      const misspelled = words.filter(x => spellchecker.isMisspelled(x))
      callback(misspelled)
    }, 0)
  }
})

webFrame.insertText('text')

webFrame.executeJavaScript('return true;').then((v: boolean) => console.log(v))
webFrame.executeJavaScript('return true;', true).then((v: boolean) => console.log(v))
webFrame.executeJavaScript('return true;', true)
webFrame.executeJavaScript('return true;', true).then((result: boolean) => console.log(result))

console.log(webFrame.getResourceUsage())
webFrame.clearCache()

// clipboard
// https://github.com/electron/electron/blob/master/docs/api/clipboard.md

clipboard.writeText('Example String')
clipboard.writeText('Example String', 'selection')
console.log(clipboard.readText('selection'))
console.log(clipboard.availableFormats())
clipboard.clear()

clipboard.write({
  html: '<html></html>',
  text: 'Hello World!',
  bookmark: 'Bookmark name',
  image: clipboard.readImage()
})

// crash-reporter
// https://github.com/electron/electron/blob/master/docs/api/crash-reporter.md

crashReporter.start({
  productName: 'YourName',
  companyName: 'YourCompany',
  submitURL: 'https://your-domain.com/url-to-submit',
  uploadToServer: true
})

// desktopCapturer
// https://github.com/electron/electron/blob/master/docs/api/desktop-capturer.md

desktopCapturer.getSources({ types: ['window', 'screen'] }).then(sources => {
  for (let i = 0; i < sources.length; ++i) {
    if (sources[i].name == 'Electron') {
      (navigator as any).webkitGetUserMedia({
        audio: false,
        video: {
          mandatory: {
            chromeMediaSource: 'desktop',
            chromeMediaSourceId: sources[i].id,
            minWidth: 1280,
            maxWidth: 1280,
            minHeight: 720,
            maxHeight: 720
          }
        }
      }, gotStream, getUserMediaError)
      return
    }
  }
})

function gotStream (stream: any) {
  (document.querySelector('video') as HTMLVideoElement).src = URL.createObjectURL(stream)
}

function getUserMediaError (error: Error) {
  console.log('getUserMediaError', error)
}

// File object
// https://github.com/electron/electron/blob/master/docs/api/file-object.md

/*
<div id="holder">
  Drag your file here
</div>
*/

const holder = document.getElementById('holder')

holder.ondragover = function () {
  return false
}

holder.ondragleave = holder.ondragend = function () {
  return false
}

holder.ondrop = function (e) {
  e.preventDefault()
  const file = e.dataTransfer.files[0]
  console.log('File you dragged here is', file.path)
  return false
}

// nativeImage
// https://github.com/electron/electron/blob/master/docs/api/native-image.md

const Tray = remote.Tray
const appIcon2 = new Tray('/Users/somebody/images/icon.png')
const window2 = new BrowserWindow({ icon: '/Users/somebody/images/window.png' })
const image = clipboard.readImage()
const appIcon3 = new Tray(image)
const appIcon4 = new Tray('/Users/somebody/images/icon.png')

// https://github.com/electron/electron/blob/master/docs/api/process.md

// preload.js
const _setImmediate = setImmediate
const _clearImmediate = clearImmediate
process.once('loaded', function () {
  global.setImmediate = _setImmediate
  global.clearImmediate = _clearImmediate
})

// screen
// https://github.com/electron/electron/blob/master/docs/api/screen.md

const app = remote.app

let mainWindow: Electron.BrowserWindow = null

app.whenReady().then(() => {
  const size = screen.getPrimaryDisplay().workAreaSize
  mainWindow = new BrowserWindow({ width: size.width, height: size.height })
})

app.whenReady().then(() => {
  const displays = screen.getAllDisplays()
  let externalDisplay: any = null
  for (const i in displays) {
    if (displays[i].bounds.x > 0 || displays[i].bounds.y > 0) {
      externalDisplay = displays[i]
      break
    }
  }

  if (externalDisplay) {
    mainWindow = new BrowserWindow({
      x: externalDisplay.bounds.x + 50,
      y: externalDisplay.bounds.y + 50
    })
  }
})

// shell
// https://github.com/electron/electron/blob/master/docs/api/shell.md

shell.openExternal('https://github.com').then(() => {})

// <webview>
// https://github.com/electron/electron/blob/master/docs/api/web-view-tag.md

const webview = document.createElement('webview')
webview.loadURL('https://github.com')

webview.addEventListener('console-message', function (e) {
  console.log('Guest page logged a message:', e.message)
})

webview.addEventListener('found-in-page', function (e) {
  if (e.result.finalUpdate) {
    webview.stopFindInPage('keepSelection')
  }
})

const requestId = webview.findInPage('test')

webview.addEventListener('new-window', async e => {
  const { shell } = require('electron')
  await shell.openExternal(e.url)
})

webview.addEventListener('close', function () {
  webview.src = 'about:blank'
})

// In embedder page.
webview.addEventListener('ipc-message', function (event) {
  console.log(event.channel) // Prints "pong"
})
webview.send('ping')
webview.capturePage().then(image => { console.log(image) })

{
  const opened: boolean = webview.isDevToolsOpened()
  const focused: boolean = webview.isDevToolsFocused()
}

// In guest page.
ipcRenderer.on('ping', function () {
  ipcRenderer.sendToHost('pong')
})
