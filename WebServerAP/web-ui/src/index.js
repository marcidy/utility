const { SmoothieChart, TimeSeries } = require('smoothie');

const out = document.getElementById("raw-websocket");
const socket = new WebSocket("ws://192.168.1.1:81");

const canvas_water = document.getElementById("water_canvas");
const canvas_power = document.getElementById("power_canvas");

const smoothie_water = new SmoothieChart();
const smoothie_power = new SmoothieChart();

const pwm_line = new TimeSeries();
const anmeter_line = new TimeSeries();

smoothie_water.addTimeSeries(anmeter_line, { lineWidth:2, strokeStyle:'#0000ff' });
smoothie_water.streamTo(canvas_water, 500);

smoothie_power.addTimeSeries(pwm_line, { lineWidth: 2, strokeStyle: '#00ff00' });
smoothie_power.streamTo(canvas_power, 500);

resize();

function resize() {
	canvas_water.setAttribute('width', window.innerWidth);
	canvas_power.setAttribute('width', window.innerWidth);
}

socket.onmessage = function(event) {
	out.textContent = String(event.data);

	let data = {};

	try {
		data = JSON.parse(event.data);
 	} catch (error) {
		return; 
	}

	const pwm = data.pwm;
	const anmeter = data.anmeter;
	const now = new Date().getTime(); // TODO: replace with time from ESP chip *!*
	pwm_line.append(now, pwm);
	anmeter_line.append(now, anmeter);
}


