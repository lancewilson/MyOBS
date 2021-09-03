// This file is required by the index.html file and will
// be executed in the renderer process for that window.
// No Node.js APIs are available in this process because
// `nodeIntegration` is turned off. Use `preload.js` to
// selectively enable features needed in the rendering
// process.

let local_file = document.getElementById('local-file')
let key = document.getElementById('key')
let start_button = document.getElementById('start_button')

start_button.disabled = true;
document.getElementById('stop_button').disabled = true;

local_file.addEventListener('input', updateStart)
key.addEventListener('input', updateStart)

function updateStart(e) {
    start_button.disabled = (local_file.value.length == 0 || key.value.length == 0)
}
