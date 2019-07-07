document.write('success')

const socket = new WebSocket("ws://192.168.1.1:81");
const out = document.body;

socket.onmessage = function(event) {
  console.debug("WebSocket message received:", event);
  out.textContent = String(event.data);
}
