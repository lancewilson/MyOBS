const { ipcRenderer } = require('electron')

let start_button = null
let stop_button = null

window.addEventListener('DOMContentLoaded', () => {
    const replaceText = (selector, text) => {
        const element = document.getElementById(selector)
        if (element) element.innerText = text
    }

    for (const dependency of ['chrome', 'node', 'electron']) {
        replaceText(`${dependency}-version`, process.versions[dependency])
    }

    start_button = document.getElementById("start_button")
    start_button.addEventListener("click", onStart)

    stop_button = document.getElementById("stop_button")
    stop_button.addEventListener("click", onStop)

    ipcRenderer.on('start-reply', (event, arg) => {
        stop_button.disabled = !arg

        if (!arg) {
            window.alert("Failed to stream output!")
        }
    })

//    window.alert(ipcRenderer.sendSync('synchronous-message', 'sync ping'))
    //process.obs.loadModules();
    //replaceText(`node-version`, process.myText)
    //process.obs.resetAudio();
})

function onStart() {
    //window.alert(ipcRenderer.sendSync('synchronous-message', 'sync ping'))
    var keyValue = document.getElementById('key').value;
    var localFileValue = document.getElementById('local-file').value;
    var args = { key: keyValue, local_file: localFileValue }

    // Async message sender
    ipcRenderer.send('start-message', args)

    //window.alert("start")
}

function onStop() {
    ipcRenderer.send('stop-message')
    stop_button.disabled = true
}