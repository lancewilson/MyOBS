const { app, BrowserWindow } = require('electron')
const { ipcMain } = require('electron')
const path = require('path')

var createOBS = require('bindings')('my-obs-addon')

let obs = null

function createWindow() {

    const win = new BrowserWindow({
        width: 800,
        height: 600,
        webPreferences: {
            preload: path.join(__dirname, 'preload.js')
        }
    })

    win.loadFile('index.html')
}

app.whenReady().then(() => {
    const videoParams = {
        width: 800,
        height: 600,
    }

    try {
        obs = createOBS();

        obs.startup("en-US")
        obs.resetVideo(videoParams)
        obs.resetAudio()    // Use defaults
        obs.loadModules();
    }
    catch (e) {
        console.log(e.description)
        obs = null
    }
    
    createWindow()

    app.on('activate', function () {
        if (BrowserWindow.getAllWindows().length === 0) createWindow()
    })
})

app.on('window-all-closed', function () {
    if (process.platform !== 'darwin') app.quit()
})

// Event handler for asynchronous incoming messages
ipcMain.on('start-message', (event, arg) => {
    if (obs != null) {
        obs.setOutputKey(arg.key)
        obs.addMediaSource(arg.local_file)
        event.sender.send('start-reply', obs.startStreaming())
    }
    else {
        event.sender.send('start-reply', false)
    }
})

// Event handler for synchronous incoming messages
ipcMain.on('stop-message', (event, arg) => {
    obs.stopStreaming();
})
