/*
  ESP32 Robotic Car Controller - BRINKO Motora
  Copyright (C) 2024 [Tu Nombre/Compania]

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// Configuracion de Pines
const int motorPins[8] = {8, 9, 6, 7, 2, 1, 4, 3}; // A1,B1,A2,B2,A3,B3,A4,B4
const int pinServo = 0;
const int pinSensorLinea = 20;

// Variables Globales
Servo direccion;
WebServer server(80);
volatile unsigned long contadorVueltas = 0;
bool ultimoEstado = HIGH;
String mensaje = "";
unsigned long tiempoMensaje = 0;
const char* password = "brinko100";

// Estados de motores
bool motorStates[4] = {false, false, false, false};
bool motorDirections[4] = {true, true, true, true};

// Variables para condiciones
bool sensorLineaEstado = HIGH;
bool condicionIfActiva = false;
unsigned int valorCondicion = 0;
String tipoCondicion = "laps"; // laps o sensor
String operadorCondicion = ">="; // >=, <=, ==, >, <
bool condicionCumplida = false;

// Configuración WiFi optimizada para ESP32-C3
#define WIFI_CHANNEL 6          // Canal menos congestionado (1, 6, 11 son mejores)
#define MAX_CLIENTS 4           // Máximo número de dispositivos conectados

// HTML Interfaz (igual que antes, no cambia)
const char* html = R"rawliteral(
<!DOCTYPE html><html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta charset="UTF-8">
<style>
body{margin:0;font-family:Poppins,sans-serif;background:#0f0f0f;color:#f2f2f2;text-align:center;}
h2{color:#C93287;margin:20px 0 10px;}
#tabs{display:flex;justify-content:center;gap:10px;margin:15px 0;}
.tab{padding:10px 18px;border-radius:20px;border:2px solid #C93287;background:transparent;color:#C93287;cursor:pointer;}
.tab.active{background:#C93287;color:white;box-shadow:0 0 12px rgba(201,50,135,0.6);}
.tab-content{margin-top:20px;}
#joystick{width:240px;height:240px;background:radial-gradient(circle at 30% 30%,#444,#111);
  border-radius:50%;border:3px solid #C93287;box-shadow:0 0 30px rgba(201,50,135,0.6);
  position:relative;margin:25px auto 10px;touch-action:none;}
#knob{width:70px;height:70px;background:radial-gradient(circle,#C93287,#680046);border-radius:50%;
  border:2px solid #fff;box-shadow:0 0 25px rgba(201,50,135,0.9);position:absolute;touch-action:none;transition:0.08s;left:85px;top:85px;}
#controls-dir{display:flex;justify-content:center;gap:15px;margin:20px;flex-wrap:wrap;}
.dir-btn{width:60px;height:60px;font-size:28px;border:none;border-radius:12px;background:#C93287;color:white;cursor:pointer;}
.dir-btn:active{transform:scale(0.95);}
.queue-container{width:95%;max-width:900px;margin:20px auto;background:#1a1a1a;border-radius:15px;padding:15px;border:2px solid #333;min-height:300px;overflow:auto;}
.queue-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:15px;padding-bottom:10px;border-bottom:2px solid #333;}
.queue-title{font-size:20px;color:#C93287;}
.queue-stats{font-size:14px;color:#aaa;}
#queue{display:flex;flex-direction:column;gap:8px;min-height:200px;}
.queue-row{display:flex;align-items:center;gap:10px;padding:12px;background:#222;border-radius:10px;transition:all 0.3s;position:relative;}
.queue-row:hover{background:#2a2a2a;}
.queue-row.active{background:linear-gradient(90deg, #222, #00ff88);animation:pulse 2s infinite;}
@keyframes pulse{0%{box-shadow:0 0 5px rgba(0,255,136,0.5);}50%{box-shadow:0 0 15px rgba(0,255,136,0.8);}100%{box-shadow:0 0 5px rgba(0,255,136,0.5);}}
.queue-row.executed{background:#333;opacity:0.7;}
.queue-content{flex-grow:1;display:flex;align-items:center;gap:15px;}
.queue-symbol{width:50px;height:50px;border-radius:10px;display:flex;align-items:center;justify-content:center;font-size:28px;font-weight:bold;flex-shrink:0;position:relative;}
.queue-symbol.motor{background:#FF9800;}
.queue-symbol.servo{background:#FF5722;}
.queue-symbol.loop-start{background:#9C27B0;}
.queue-symbol.loop-end{background:#673AB7;}
.queue-symbol.pause{background:#607D8B;}
.queue-symbol.if-start{background:#4CAF50;}
.queue-symbol.if-end{background:#2196F3;}
.queue-details{display:flex;flex-direction:column;gap:3px;text-align:left;flex-grow:1;}
.queue-label{font-size:16px;font-weight:bold;}
.queue-info{font-size:12px;color:#aaa;}
.queue-actions{display:flex;gap:5px;}
.action-btn{width:35px;height:35px;border:none;border-radius:6px;background:#333;color:white;cursor:pointer;display:flex;align-items:center;justify-content:center;}
.action-btn:hover{background:#444;}
.action-btn.delete{background:#f44336;}
.action-btn.delete:hover{background:#ff5555;}
.action-btn.move{background:#555;}
#controls-main{margin-top:15px;}
.ctrl-btn{margin:8px;padding:12px 24px;font-size:16px;border:none;border-radius:10px;background:#C93287;color:white;cursor:pointer;min-width:100px;}
.ctrl-btn:hover{background:#d9368f;}
#panel{margin-top:20px;}
#mensaje{margin-top:10px;font-size:18px;color:#ffb7ff;}
.welcome-modal,.modal{position:fixed;inset:0;background:rgba(0,0,0,0.75);display:flex;justify-content:center;align-items:center;z-index:9999;}
.modal-box{background:#111;border:2px solid #C93287;border-radius:14px;padding:20px;width:340px;text-align:center;
  box-shadow:0 0 25px rgba(201,50,135,0.6);}
.hidden{display:none;}
.motor-control-row{display:flex;justify-content:center;align-items:center;gap:10px;margin:10px 0;}
.motor-number{width:40px;height:40px;border-radius:50%;background:#222;color:#C93287;display:flex;align-items:center;justify-content:center;font-size:18px;font-weight:bold;}
.motor-direction-btn{width:80px;height:35px;border:none;border-radius:6px;cursor:pointer;font-size:16px;font-weight:bold;}
.motor-forward-btn{background:#4CAF50;color:white;}
.motor-backward-btn{background:#2196F3;color:white;}
.motor-direction-btn.selected{box-shadow:0 0 0 2px white;}
.motor-direction-btn.active{opacity:0.7;}
.action-btn-modal{width:120px;height:40px;margin:10px;border:none;border-radius:8px;cursor:pointer;font-size:14px;}
.test-btn{background:#FF9800;color:white;}
.stop-btn{background:#f44336;color:white;}
.time-input{width:80px;height:35px;font-size:16px;text-align:center;background:#222;color:white;border:1px solid #C93287;border-radius:6px;margin:5px;}
.servo-input{width:100px;height:35px;font-size:16px;text-align:center;background:#222;color:white;border:1px solid #FF5722;border-radius:6px;margin:5px;}
.selected-motors-info{font-size:14px;margin:15px 0;color:#4CAF50;min-height:20px;}
.control-section{margin:15px 0;}
.control-section h4{margin-bottom:8px;color:#C93287;}
.time-section{display:flex;align-items:center;justify-content:center;gap:10px;margin:15px 0;}
.command-palette{display:flex;justify-content:center;gap:15px;margin:15px 0;flex-wrap:wrap;}
.command-btn{width:70px;height:70px;border-radius:12px;display:flex;align-items:center;justify-content:center;font-size:32px;cursor:pointer;border:2px solid transparent;transition:all 0.2s;flex-direction:column;gap:5px;}
.command-btn:hover{transform:scale(1.05);border-color:white;}
.command-btn-label{font-size:11px;opacity:0.8;}
#cmd-motor{background:#FF9800;color:white;}
#cmd-servo{background:#FF5722;color:white;}
#cmd-pause{background:#607D8B;color:white;}
#cmd-loop-start{background:#9C27B0;color:white;}
#cmd-if-start{background:#4CAF50;color:white;}
.empty-queue{display:flex;flex-direction:column;align-items:center;justify-content:center;padding:50px;color:#666;font-style:italic;}
.empty-queue-icon{font-size:48px;margin-bottom:10px;opacity:0.5;}
.loop-indent{margin-left:30px;border-left:2px dashed #9C27B0;padding-left:10px;}
.if-indent{margin-left:30px;border-left:2px dashed #4CAF50;padding-left:10px;}
.loop-block{position:relative;}
.loop-block::before{content:'';position:absolute;left:-20px;top:0;bottom:0;width:2px;background:#9C27B0;}
.if-block{position:relative;}
.if-block::before{content:'';position:absolute;left:-20px;top:0;bottom:0;width:2px;background:#4CAF50;}
.loop-end-modal .time-input{width:120px;}
.if-modal .time-input{width:120px;}
.position-badge{position:absolute;top:-8px;right:-8px;background:#C93287;color:white;border-radius:50%;width:22px;height:22px;font-size:12px;display:flex;align-items:center;justify-content:center;font-weight:bold;z-index:10;}
.position-badge.loop-start{background:#9C27B0;}
.position-badge.loop-end{background:#673AB7;}
.position-badge.if-start{background:#4CAF50;}
.position-badge.if-end{background:#2196F3;}
.slider-container{margin:15px 0;padding:0 10px;}
.slider-value{font-size:16px;color:#C93287;margin:5px 0;font-weight:bold;}
.servo-slider-container{margin:15px 0;}
.pause-slider-container{margin:15px 0;}
.queue-index{width:40px;height:40px;border-radius:50%;background:#333;color:#C93287;display:flex;align-items:center;justify-content:center;font-size:18px;font-weight:bold;flex-shrink:0;}
.condition-selector{margin:15px 0;display:flex;justify-content:center;gap:10px;flex-wrap:wrap;}
.condition-option{padding:10px 15px;border-radius:8px;border:2px solid #4CAF50;background:#222;color:#4CAF50;cursor:pointer;}
.condition-option.selected{background:#4CAF50;color:white;}
.condition-type-selector{display:flex;justify-content:center;gap:15px;margin:10px 0;}
.condition-type-btn{padding:8px 15px;border-radius:6px;border:2px solid #4CAF50;background:#222;color:#4CAF50;cursor:pointer;}
.condition-type-btn.selected{background:#4CAF50;color:white;}
.operator-selector{display:flex;justify-content:center;gap:10px;margin:15px 0;flex-wrap:wrap;}
.operator-btn{padding:8px 12px;border-radius:6px;border:2px solid #2196F3;background:#222;color:#2196F3;cursor:pointer;}
.operator-btn.selected{background:#2196F3;color:white;}
.sensor-info{margin-top:10px;font-size:12px;color:#aaa;font-style:italic;}
.warning-banner{background:#ff9800;color:#333;padding:10px;border-radius:8px;margin:10px 0;display:none;font-weight:bold;border:2px solid #ff5722;}
.warning-banner.show{display:block;}
.auto-close-btn{background:#ff9800;color:#333;border:none;padding:8px 15px;border-radius:6px;cursor:pointer;font-weight:bold;margin:5px;}
.auto-close-btn:hover{background:#ffb74d;}
</style>
</head>
<body>
<div id="welcomeModal" class="welcome-modal">
  <div class="modal-box">
    <h2>&#x1F916; BRINKO Motora</h2>
    <p>Controla tu robot en modo manual o programa movimientos.</p>
    <p><small>Las llaves abiertas se cerrarán automáticamente al ejecutar</small></p>
    <button onclick="closeWelcome()" style="margin-top:15px;padding:10px 18px;border:none;border-radius:14px;background:#C93287;color:white;cursor:pointer;">OK</button>
  </div>
</div>

<div id="tabs">
  <button class="tab active" onclick="showTab('manual')">&#x1F579; Manual</button>
  <button class="tab" onclick="showTab('program')">&#x1F4BB; Programa</button>
</div>

<div id="tab-manual" class="tab-content">
  <div id="joystick"><div id="knob"></div></div>
  <div id="panel">
    <h3>LAP: <span id="vueltas">0</span></h3>
    <button class="dir-btn" onclick="resetLaps()">&#x21BA;</button>
  </div>
  <h3 id="mensaje"></h3>
</div>

<div id="tab-program" class="tab-content hidden">
  <div id="warningBanner" class="warning-banner">
    <span id="warningText"></span>
    <button class="auto-close-btn" onclick="autoCloseBlocks()">Cerrar automáticamente</button>
    <button class="auto-close-btn" onclick="closeWarning()" style="background:#f44336;color:white;">Ignorar</button>
  </div>
  
  <div class="command-palette">
    <button id="cmd-motor" class="command-btn" onclick="openMotorModal()">
      M
      <span class="command-btn-label">Motor</span>
    </button>
    <button id="cmd-servo" class="command-btn" onclick="openServoModal()">
      &#x21C4;
      <span class="command-btn-label">Servo</span>
    </button>
    <button id="cmd-pause" class="command-btn" onclick="openPauseModal()">
      &#9208;
      <span class="command-btn-label">Pausa</span>
    </button>
    <button id="cmd-loop-start" class="command-btn" onclick="openLoopModal()">
      🔁
      <span class="command-btn-label">Loop</span>
    </button>
    <button id="cmd-if-start" class="command-btn" onclick="openIfModal()">
      ⚡
      <span class="command-btn-label">Condición</span>
    </button>
    <button class="command-btn" onclick="closeBlock()" style="background:#673AB7;">
      }
      <span class="command-btn-label">Cerrar Bloque</span>
    </button>
  </div>
  
  <div class="queue-container">
    <div class="queue-header">
      <div class="queue-title">&#128221; Comandos</div>
      <div class="queue-stats" id="queueStats">Total: 0 | Duración: 0ms</div>
    </div>
    <div id="queue"></div>
  </div>
  
  <div id="controls-main">
    <button class="ctrl-btn" onclick="playQueue()">&#x25B6; Ejecutar</button>
    <button class="ctrl-btn" onclick="pauseQueue()">&#9208; Pausar</button>
    <button class="ctrl-btn" onclick="stopQueue()">&#x23F9; Detener</button>
    <button class="ctrl-btn" onclick="clearQueue()">&#x1F5D1; Limpiar</button>
  </div>
</div>

<div id="motorModal" class="modal hidden">
  <div class="modal-box">
    <h3>Control de Motores</h3>
    <div class="motor-control-row">
      <div class="motor-number">1</div>
      <button class="motor-direction-btn motor-forward-btn" onclick="selectMotor(1,'forward')">adelante</button>
      <button class="motor-direction-btn motor-backward-btn" onclick="selectMotor(1,'backward')">atrás</button>
    </div>
    <div class="motor-control-row">
      <div class="motor-number">2</div>
      <button class="motor-direction-btn motor-forward-btn" onclick="selectMotor(2,'forward')">adelante</button>
      <button class="motor-direction-btn motor-backward-btn" onclick="selectMotor(2,'backward')">atrás</button>
    </div>
    <div class="motor-control-row">
      <div class="motor-number">3</div>
      <button class="motor-direction-btn motor-forward-btn" onclick="selectMotor(3,'forward')">adelante</button>
      <button class="motor-direction-btn motor-backward-btn" onclick="selectMotor(3,'backward')">atrás</button>
    </div>
    <div class="motor-control-row">
      <div class="motor-number">4</div>
      <button class="motor-direction-btn motor-forward-btn" onclick="selectMotor(4,'forward')">adelante</button>
      <button class="motor-direction-btn motor-backward-btn" onclick="selectMotor(4,'backward')">atrás</button>
    </div>
    <div id="selectedMotorsInfo" class="selected-motors-info">Selecciona motores</div>
    <div class="time-section">
      <span>Tiempo (ms):</span>
      <input id="motorTime" type="number" min="100" max="10000" value="800" class="time-input">
    </div>
    
    <div>
      <button class="action-btn-modal" style="background:#4CAF50;" onclick="addMotorCommand()">Añadir Comando</button>
      <button class="action-btn-modal test-btn" onclick="testMotors()">Probar Motores</button>
    </div>
    <div style="margin-top:20px;">
      <button onclick="clearSelections()" style="padding:8px 16px;margin:5px;border:none;border-radius:8px;background:#f44336;color:white;cursor:pointer;font-size:13px;">Limpiar</button>
      <button onclick="closeMotorModal()" style="padding:10px 24px;margin-top:10px;border:none;border-radius:10px;background:#C93287;color:white;cursor:pointer;">Cerrar</button>
    </div>
  </div>
</div>

<div id="servoModal" class="modal hidden">
  <div class="modal-box">
    <h3>Comando de Servo</h3>
    
    <div class="slider-container">
      <input type="range" id="servoSlider" min="55" max="125" value="90" step="1" style="width:100%;">
      <div class="slider-value">Ángulo: <span id="servoAngleValue">90</span>°</div>
    </div>
    
    <div class="time-section">
      <span>Ángulo (55-125):</span>
      <input id="servoAngleInput" type="number" min="55" max="125" value="90" class="servo-input" onchange="updateServoSlider(this.value)">
    </div>
    
    <div class="time-section">
      <span>Duración (ms):</span>
      <input id="servoDuration" type="number" min="100" max="5000" value="500" class="time-input">
    </div>
    
    <div style="margin-top:15px;">
      <button onclick="addServoCommand()" style="margin:6px;padding:8px 14px;border-radius:10px;border:none;background:#FF5722;color:white;">Añadir Comando</button>
      <button onclick="testServoCommand()" style="margin:6px;padding:8px 14px;border-radius:10px;border:none;background:#FF9800;color:white;">Probar Ahora</button>
      <button onclick="closeServoModal()" style="margin:6px;padding:8px 14px;border-radius:10px;border:none;background:#f44336;color:white;">Cancelar</button>
    </div>
  </div>
</div>

<div id="pauseModal" class="modal hidden">
  <div class="modal-box">
    <h3>Comando de Pausa</h3>
    
    <div class="slider-container">
      <input type="range" id="pauseSlider" min="0" max="3000" value="1000" step="100" style="width:100%;">
      <div class="slider-value">Duración: <span id="pauseSliderValue">1000</span>ms</div>
    </div>
    
    <div class="time-section">
      <span>Duración (ms):</span>
      <input id="pauseDuration" type="number" min="0" max="30000" value="1000" class="time-input" onchange="updatePauseSlider(this.value)">
    </div>
    
    <div style="margin-top:15px;">
      <button onclick="addPauseCommand()" style="margin:6px;padding:8px 14px;border-radius:10px;border:none;background:#607D8B;color:white;">Añadir Pausa</button>
      <button onclick="closePauseModal()" style="margin:6px;padding:8px 14px;border-radius:10px;border:none;background:#f44336;color:white;">Cancelar</button>
    </div>
  </div>
</div>

<div id="loopModal" class="modal hidden">
  <div class="modal-box">
    <h3>Bucle (Loop)</h3>
    <div class="time-section">
      <span>Repeticiones:</span>
      <input id="loopRepetitions" type="number" min="2" max="99" value="2" class="time-input">
    </div>
    <div class="sensor-info">El bloque se ejecutará N veces</div>
    <div style="margin-top:15px;">
      <button onclick="addLoopStart()" style="margin:6px;padding:8px 14px;border-radius:10px;border:none;background:#4CAF50;color:white;">Añadir Loop</button>
      <button onclick="closeLoopModal()" style="margin:6px;padding:8px 14px;border-radius:10px;border:none;background:#f44336;color:white;">Cancelar</button>
    </div>
  </div>
</div>

<div id="ifModal" class="modal hidden if-modal">
  <div class="modal-box">
    <h3>Condición IF</h3>
    
    <div class="condition-type-selector">
      <button class="condition-type-btn" onclick="selectConditionType('laps')" id="btnCondLaps">LAPS</button>
      <button class="condition-type-btn" onclick="selectConditionType('sensor')" id="btnCondSensor">SENSOR</button>
    </div>
    
    <div id="conditionLaps" class="condition-section">
      <div class="operator-selector">
        <button class="operator-btn" onclick="selectOperator('>=')">≥</button>
        <button class="operator-btn" onclick="selectOperator('<=')">≤</button>
        <button class="operator-btn" onclick="selectOperator('==')">=</button>
        <button class="operator-btn" onclick="selectOperator('>')">></button>
        <button class="operator-btn" onclick="selectOperator('<')"><</button>
      </div>
      <div class="time-section">
        <span>Valor LAPS:</span>
        <input id="ifValue" type="number" min="0" max="99" value="1" class="time-input">
      </div>
      <div class="sensor-info">Se ejecutará si LAPS cumple la condición</div>
    </div>
    
    <div id="conditionSensor" class="condition-section hidden">
      <div class="condition-selector">
        <button class="condition-option" onclick="selectSensorCondition('HIGH')">SENSOR HIGH</button>
        <button class="condition-option" onclick="selectSensorCondition('LOW')">SENSOR LOW</button>
      </div>
      <div class="sensor-info">Se ejecutará cuando el sensor esté en el estado seleccionado</div>
    </div>
    
    <div style="margin-top:15px;">
      <button onclick="addIfStart()" style="margin:6px;padding:8px 14px;border-radius:10px;border:none;background:#4CAF50;color:white;">Añadir IF</button>
      <button onclick="closeIfModal()" style="margin:6px;padding:8px 14px;border-radius:10px;border:none;background:#f44336;color:white;">Cancelar</button>
    </div>
  </div>
</div>

<script>
let commandQueue = [];
let isPlaying = false;
let currentIndex = 0;
let motorSelections = {1:null, 2:null, 3:null, 4:null};
let executedCommands = new Set();
let ifConditionType = 'laps';
let ifOperator = '>=';
let ifValue = 1;
let sensorCondition = 'HIGH';
let conditionValue = 1;
let blocksAutoClosed = false;

// Generar ID único
function generateId() {
  return 'id_' + Date.now() + '_' + Math.random().toString(36).substr(2, 9);
}

// Joystick
const knob = document.getElementById('knob');
const joystick = document.getElementById('joystick');
let dragging = false;
let lastSend = 0;

function updateJoystick(x, y) {
  const maxDist = 70;
  const dist = Math.sqrt(x*x + y*y);
  if(dist > maxDist) {
    x = x * maxDist / dist;
    y = y * maxDist / dist;
  }
  
  knob.style.left = (85 + x) + 'px';
  knob.style.top = (85 + y) + 'px';
  
  const normX = Math.round((x/maxDist)*100);
  const normY = Math.round((-y/maxDist)*100);
  
  const now = Date.now();
  if(now - lastSend >= 50) {
    lastSend = now;
    fetch('/control?x=' + normX + '&y=' + normY);
  }
}

knob.addEventListener('pointerdown', (e) => {
  dragging = true;
  knob.setPointerCapture(e.pointerId);
});

knob.addEventListener('pointermove', (e) => {
  if(!dragging) return;
  const rect = joystick.getBoundingClientRect();
  const x = e.clientX - rect.left - 120;
  const y = e.clientY - rect.top - 120;
  updateJoystick(x, y);
});

knob.addEventListener('pointerup', () => {
  dragging = false;
  knob.style.left = '85px';
  knob.style.top = '85px';
  fetch('/control?x=0&y=0');
});

// Control de sliders
function updateServoSlider(value) {
  const slider = document.getElementById('servoSlider');
  const display = document.getElementById('servoAngleValue');
  
  value = Math.min(125, Math.max(55, parseInt(value) || 90));
  slider.value = value;
  display.textContent = value;
  
  // Testear posición actual
  fetch('/servo?angle=' + value);
}

function updatePauseSlider(value) {
  const slider = document.getElementById('pauseSlider');
  const display = document.getElementById('pauseSliderValue');
  
  value = parseInt(value) || 1000;
  slider.value = Math.min(3000, value);
  display.textContent = value;
}

// Inicializar sliders
document.addEventListener('DOMContentLoaded', function() {
  const servoSlider = document.getElementById('servoSlider');
  const pauseSlider = document.getElementById('pauseSlider');
  
  servoSlider.addEventListener('input', function() {
    const value = this.value;
    document.getElementById('servoAngleValue').textContent = value;
    document.getElementById('servoAngleInput').value = value;
  });
  
  pauseSlider.addEventListener('input', function() {
    const value = this.value;
    document.getElementById('pauseSliderValue').textContent = value;
    document.getElementById('pauseDuration').value = value;
  });
  
  // Inicializar selecciones
  selectConditionType('laps');
  selectOperator('>=');
  selectSensorCondition('HIGH');
});

// Motor control
function selectMotor(num, dir) {
  const btnF = document.querySelector(`button[onclick="selectMotor(${num},'forward')"]`);
  const btnB = document.querySelector(`button[onclick="selectMotor(${num},'backward')"]`);
  
  if(motorSelections[num] === dir) {
    motorSelections[num] = null;
    btnF.classList.remove('selected');
    btnB.classList.remove('selected');
  } else {
    motorSelections[num] = dir;
    btnF.classList.remove('selected');
    btnB.classList.remove('selected');
    if(dir === 'forward') btnF.classList.add('selected');
    else btnB.classList.add('selected');
  }
  updateSelectedDisplay();
}

function updateSelectedDisplay() {
  const display = document.getElementById('selectedMotorsInfo');
  const selected = [];
  for(let i=1; i<=4; i++) {
    if(motorSelections[i]) {
      selected.push('M' + i + ':' + (motorSelections[i] === 'forward' ? '↑' : '↓'));
    }
  }
  display.textContent = selected.length ? selected.join(' ') : 'Selecciona motores';
}

function clearSelections() {
  for(let i=1; i<=4; i++) {
    motorSelections[i] = null;
    const btnF = document.querySelector(`button[onclick="selectMotor(${i},'forward')"]`);
    const btnB = document.querySelector(`button[onclick="selectMotor(${i},'backward')"]`);
    btnF.classList.remove('selected');
    btnB.classList.remove('selected');
  }
  updateSelectedDisplay();
}

// Añadir comando a la cola (siempre al final)
function addCommandToQueue(command) {
  commandQueue.push(command);
  renderQueue();
}

// Comando de pausa con modal
function openPauseModal() {
  document.getElementById('pauseModal').classList.remove('hidden');
}

function closePauseModal() {
  document.getElementById('pauseModal').classList.add('hidden');
}

function addPauseCommand() {
  if(isPlaying) return;
  
  const duration = parseInt(document.getElementById('pauseDuration').value) || 1000;
  
  const command = {
    id: generateId(),
    type: 'pause',
    cmd: 'pause_' + duration,
    duration: duration,
    symbol: '⏸️',
    label: 'Pausa',
    info: duration + 'ms',
    level: getCurrentIndentLevel()
  };
  
  addCommandToQueue(command);
  closePauseModal();
}

// Comando de motores
function addMotorCommand() {
  if(isPlaying) return;
  const duration = parseInt(document.getElementById('motorTime').value) || 800;
  const motors = [];
  const forward = [], backward = [];
  
  for(let i=1; i<=4; i++) {
    if(motorSelections[i]) {
      motors.push(i);
      if(motorSelections[i] === 'forward') forward.push(i);
      else backward.push(i);
    }
  }
  
  if(motors.length === 0) {
    alert('Selecciona al menos un motor');
    return;
  }
  
  let cmd = 'mixedmotor_';
  if(forward.length) cmd += 'forward' + forward.join(',');
  if(backward.length) {
    if(forward.length) cmd += '_';
    cmd += 'backward' + backward.join(',');
  }
  cmd += '_time' + duration;
  
  let label = 'Motores: ';
  let details = '';
  if(forward.length) {
    label += 'Adelante ' + forward.join(',');
    details += '↑' + forward.join(',');
  }
  if(backward.length) {
    if(forward.length) {
      label += ', Atrás ' + backward.join(',');
      details += ' ↓' + backward.join(',');
    } else {
      label += 'Atrás ' + backward.join(',');
      details += '↓' + backward.join(',');
    }
  }
  
  const command = {
    id: generateId(),
    type: 'motor',
    cmd: cmd,
    motors: motors,
    forward: forward,
    backward: backward,
    duration: duration,
    symbol: 'M',
    label: label,
    info: details + ' | ' + duration + 'ms',
    level: getCurrentIndentLevel()
  };
  
  addCommandToQueue(command);
  closeMotorModal();
}

// Comando de servo
function addServoCommand() {
  if(isPlaying) return;
  const angle = parseInt(document.getElementById('servoAngleInput').value) || 90;
  const duration = parseInt(document.getElementById('servoDuration').value) || 500;
  
  const command = {
    id: generateId(),
    type: 'servo',
    cmd: 'servo_' + angle + '_time' + duration,
    angle: angle,
    duration: duration,
    symbol: '↔',
    label: 'Mover Servo',
    info: angle + '° por ' + duration + 'ms',
    level: getCurrentIndentLevel()
  };
  
  addCommandToQueue(command);
  closeServoModal();
}

function testServoCommand() {
  const angle = parseInt(document.getElementById('servoAngleInput').value) || 90;
  fetch('/servo?angle=' + angle);
}

async function testMotors() {
  const duration = parseInt(document.getElementById('motorTime').value) || 800;
  for(let i=1; i<=4; i++) {
    if(motorSelections[i]) {
      fetch('/motor?num=' + i + '&dir=' + motorSelections[i] + '&test=true');
    }
  }
  await new Promise(r => setTimeout(r, duration));
  for(let i=1; i<=4; i++) {
    if(motorSelections[i]) {
      fetch('/motor?num=' + i + '&dir=stop');
    }
  }
}

// Sistema de bloques anidados
function getCurrentIndentLevel() {
  let level = 0;
  for(let i = 0; i < commandQueue.length; i++) {
    if(commandQueue[i].type === 'loop-start' || commandQueue[i].type === 'if-start') {
      level++;
    } else if(commandQueue[i].type === 'loop-end' || commandQueue[i].type === 'if-end') {
      level--;
    }
  }
  return level;
}

// Verificar si hay bloques abiertos
function checkOpenBlocks() {
  let openLoops = 0;
  let openIfs = 0;
  let unclosedBlocks = [];
  
  for(let item of commandQueue) {
    if(item.type === 'loop-start') openLoops++;
    else if(item.type === 'loop-end') openLoops--;
    else if(item.type === 'if-start') openIfs++;
    else if(item.type === 'if-end') openIfs--;
  }
  
  if(openLoops > 0) unclosedBlocks.push(openLoops + ' loop(s)');
  if(openIfs > 0) unclosedBlocks.push(openIfs + ' condición(es) IF');
  
  return unclosedBlocks;
}

// Mostrar advertencia de bloques abiertos
function showOpenBlocksWarning() {
  const unclosedBlocks = checkOpenBlocks();
  if(unclosedBlocks.length > 0 && !blocksAutoClosed) {
    const warningBanner = document.getElementById('warningBanner');
    const warningText = document.getElementById('warningText');
    
    warningText.textContent = '⚠️ Hay ' + unclosedBlocks.join(' y ') + ' sin cerrar. Se cerrarán automáticamente al ejecutar.';
    warningBanner.classList.add('show');
    
    return true;
  } else {
    document.getElementById('warningBanner').classList.remove('show');
    return false;
  }
}

// Cerrar bloques automáticamente
function autoCloseBlocks() {
  if(isPlaying) return;
  
  const unclosedBlocks = checkOpenBlocks();
  if(unclosedBlocks.length === 0) {
    alert('No hay bloques abiertos para cerrar');
    return;
  }
  
  // Contar cuántos bloques de cada tipo necesitamos cerrar
  let openLoops = 0;
  let openIfs = 0;
  
  for(let item of commandQueue) {
    if(item.type === 'loop-start') openLoops++;
    else if(item.type === 'loop-end') openLoops--;
    else if(item.type === 'if-start') openIfs++;
    else if(item.type === 'if-end') openIfs--;
  }
  
  // Cerrar loops
  for(let i = 0; i < openLoops; i++) {
    const command = {
      id: generateId(),
      type: 'loop-end',
      cmd: 'loop_end',
      symbol: '}',
      label: 'Fin de Loop',
      info: 'Cerrar loop',
      level: getCurrentIndentLevel()
    };
    addCommandToQueue(command);
  }
  
  // Cerrar IFs
  for(let i = 0; i < openIfs; i++) {
    const command = {
      id: generateId(),
      type: 'if-end',
      cmd: 'if_end',
      symbol: '}',
      label: 'Fin de IF',
      info: 'Cerrar condición',
      level: getCurrentIndentLevel()
    };
    addCommandToQueue(command);
  }
  
  blocksAutoClosed = true;
  document.getElementById('warningBanner').classList.remove('show');
  alert('Se cerraron ' + (openLoops + openIfs) + ' bloque(s) automáticamente.');
}

function closeWarning() {
  document.getElementById('warningBanner').classList.remove('show');
}

// Loop functions
function openLoopModal() {
  document.getElementById('loopModal').classList.remove('hidden');
}

function closeLoopModal() {
  document.getElementById('loopModal').classList.add('hidden');
}

function addLoopStart() {
  if(isPlaying) return;
  
  const repetitions = parseInt(document.getElementById('loopRepetitions').value) || 2;
  
  const command = {
    id: generateId(),
    type: 'loop-start',
    cmd: 'loop_start_' + repetitions,
    repetitions: repetitions,
    symbol: '🔁',
    label: 'Loop',
    info: 'Repetir ' + repetitions + ' veces',
    level: getCurrentIndentLevel()
  };
  
  addCommandToQueue(command);
  closeLoopModal();
  blocksAutoClosed = false;
  showOpenBlocksWarning();
}

// IF functions
function openIfModal() {
  document.getElementById('ifModal').classList.remove('hidden');
}

function closeIfModal() {
  document.getElementById('ifModal').classList.add('hidden');
}

function selectConditionType(type) {
  ifConditionType = type;
  
  // Actualizar botones
  document.querySelectorAll('.condition-type-btn').forEach(btn => {
    btn.classList.remove('selected');
  });
  document.getElementById('btnCond' + type.charAt(0).toUpperCase() + type.slice(1)).classList.add('selected');
  
  // Mostrar/ocultar secciones
  document.getElementById('conditionLaps').style.display = type === 'laps' ? 'block' : 'none';
  document.getElementById('conditionSensor').style.display = type === 'sensor' ? 'block' : 'none';
}

function selectOperator(op) {
  ifOperator = op;
  
  // Actualizar botones
  document.querySelectorAll('.operator-btn').forEach(btn => {
    btn.classList.remove('selected');
  });
  event.target.classList.add('selected');
}

function selectSensorCondition(cond) {
  sensorCondition = cond;
  
  // Actualizar botones
  document.querySelectorAll('.condition-option').forEach(btn => {
    btn.classList.remove('selected');
  });
  event.target.classList.add('selected');
}

function addIfStart() {
  if(isPlaying) return;
  
  if(ifConditionType === 'laps') {
    conditionValue = parseInt(document.getElementById('ifValue').value) || 1;
    const cmd = 'if_laps_' + ifOperator + '_' + conditionValue;
    
    const command = {
      id: generateId(),
      type: 'if-start',
      cmd: cmd,
      conditionType: 'laps',
      operator: ifOperator,
      value: conditionValue,
      symbol: '⚡',
      label: 'IF LAPS',
      info: 'LAPS ' + ifOperator + ' ' + conditionValue,
      level: getCurrentIndentLevel()
    };
    
    addCommandToQueue(command);
  } else {
    const cmd = 'if_sensor_' + sensorCondition;
    
    const command = {
      id: generateId(),
      type: 'if-start',
      cmd: cmd,
      conditionType: 'sensor',
      sensorState: sensorCondition,
      symbol: '⚡',
      label: 'IF SENSOR',
      info: 'Sensor ' + sensorCondition,
      level: getCurrentIndentLevel()
    };
    
    addCommandToQueue(command);
  }
  
  closeIfModal();
  blocksAutoClosed = false;
  showOpenBlocksWarning();
}

// Cerrar bloque
function closeBlock() {
  if(isPlaying) return;
  
  // Verificar que haya un bloque abierto
  let openCount = 0;
  let lastOpenType = null;
  for(let i = commandQueue.length - 1; i >= 0; i--) {
    if(commandQueue[i].type === 'loop-start' || commandQueue[i].type === 'if-start') {
      openCount++;
      if(openCount === 1) {
        lastOpenType = commandQueue[i].type;
        break;
      }
    } else if(commandQueue[i].type === 'loop-end' || commandQueue[i].type === 'if-end') {
      openCount--;
    }
  }
  
  if(openCount <= 0) {
    alert('No hay bloques abiertos para cerrar');
    return;
  }
  
  let command;
  if(lastOpenType === 'loop-start') {
    // Buscar el loop-start correspondiente
    for(let i = commandQueue.length - 1; i >= 0; i--) {
      if(commandQueue[i].type === 'loop-start') {
        const repetitions = commandQueue[i].repetitions || 2;
        command = {
          id: generateId(),
          type: 'loop-end',
          cmd: 'loop_end',
          symbol: '}',
          label: 'Fin de Loop',
          info: 'Cerrar loop',
          level: getCurrentIndentLevel()
        };
        break;
      }
    }
  } else if(lastOpenType === 'if-start') {
    command = {
      id: generateId(),
      type: 'if-end',
      cmd: 'if_end',
      symbol: '}',
      label: 'Fin de IF',
      info: 'Cerrar condición',
      level: getCurrentIndentLevel()
    };
  }
  
  if(command) {
    addCommandToQueue(command);
    blocksAutoClosed = false;
    showOpenBlocksWarning();
  }
}

// Renderizado de cola con bloques anidados
function renderQueue() {
  const q = document.getElementById('queue');
  const stats = document.getElementById('queueStats');
  
  if(commandQueue.length === 0) {
    q.innerHTML = `
      <div class="empty-queue">
        <div class="empty-queue-icon">📭</div>
        <div>No hay comandos</div>
        <div style="font-size:12px;margin-top:10px;">Usa los botones para añadir comandos</div>
      </div>
    `;
    stats.textContent = 'Total: 0 | Duración: 0ms';
    return;
  }
  
  q.innerHTML = '';
  let totalDuration = 0;
  let blockLevels = {};
  
  // Primera pasada: calcular niveles de bloques
  let currentLevel = 0;
  commandQueue.forEach((item, index) => {
    if(item.type === 'loop-start' || item.type === 'if-start') {
      currentLevel++;
      blockLevels[index] = currentLevel;
    } else if(item.type === 'loop-end' || item.type === 'if-end') {
      blockLevels[index] = currentLevel;
      currentLevel--;
    } else {
      blockLevels[index] = currentLevel;
    }
  });
  
  // Segunda pasada: renderizar con indentación
  commandQueue.forEach((item, index) => {
    const row = document.createElement('div');
    row.className = 'queue-row';
    
    // Calcular margen izquierdo basado en nivel
    const level = blockLevels[index] || 0;
    if(level > 0) {
      row.classList.add('loop-block');
      if(item.type === 'if-start' || (item.type !== 'loop-start' && item.type !== 'loop-end')) {
        // Dentro de un IF, usar color diferente
        row.classList.add('if-block');
      }
      row.style.marginLeft = (level * 20) + 'px';
    }
    
    // Añadir clases según estado
    if(isPlaying && index === currentIndex) {
      row.classList.add('active');
    }
    if(executedCommands.has(item.id)) {
      row.classList.add('executed');
    }
    
    // Índice
    const indexDiv = document.createElement('div');
    indexDiv.className = 'queue-index';
    indexDiv.textContent = index + 1;
    row.appendChild(indexDiv);
    
    // Contenido
    const contentDiv = document.createElement('div');
    contentDiv.className = 'queue-content';
    
    // Símbolo
    const symbolDiv = document.createElement('div');
    symbolDiv.className = 'queue-symbol ' + item.type;
    symbolDiv.textContent = item.symbol;
    
    contentDiv.appendChild(symbolDiv);
    
    // Detalles
    const detailsDiv = document.createElement('div');
    detailsDiv.className = 'queue-details';
    
    const labelDiv = document.createElement('div');
    labelDiv.className = 'queue-label';
    labelDiv.textContent = item.label || getLabelForType(item.type);
    detailsDiv.appendChild(labelDiv);
    
    const infoDiv = document.createElement('div');
    infoDiv.className = 'queue-info';
    infoDiv.textContent = item.info || getInfoForItem(item);
    detailsDiv.appendChild(infoDiv);
    
    contentDiv.appendChild(detailsDiv);
    row.appendChild(contentDiv);
    
    // Acciones para TODOS los comandos
    const actionsDiv = document.createElement('div');
    actionsDiv.className = 'queue-actions';
    
    // Botón para mover arriba
    const upBtn = document.createElement('button');
    upBtn.className = 'action-btn move';
    upBtn.innerHTML = '⬆';
    upBtn.title = 'Mover arriba';
    upBtn.onclick = (e) => {
      e.stopPropagation();
      moveCommandUp(index);
    };
    actionsDiv.appendChild(upBtn);
    
    // Botón para mover abajo
    const downBtn = document.createElement('button');
    downBtn.className = 'action-btn move';
    downBtn.innerHTML = '⬇';
    downBtn.title = 'Mover abajo';
    downBtn.onclick = (e) => {
      e.stopPropagation();
      moveCommandDown(index);
    };
    actionsDiv.appendChild(downBtn);
    
    // Botón eliminar
    const deleteBtn = document.createElement('button');
    deleteBtn.className = 'action-btn delete';
    deleteBtn.innerHTML = '🗑️';
    deleteBtn.title = 'Eliminar';
    deleteBtn.onclick = (e) => {
      e.stopPropagation();
      deleteCommandAt(index);
    };
    actionsDiv.appendChild(deleteBtn);
    
    row.appendChild(actionsDiv);
    
    q.appendChild(row);
    
    // Calcular duración total (excluyendo bloques)
    if(item.type !== 'loop-start' && item.type !== 'loop-end' && 
       item.type !== 'if-start' && item.type !== 'if-end') {
      totalDuration += item.duration || 800;
    }
  });
  
  // Actualizar estadísticas
  const regularCommands = commandQueue.filter(cmd => 
    cmd.type !== 'loop-start' && cmd.type !== 'loop-end' &&
    cmd.type !== 'if-start' && cmd.type !== 'if-end'
  ).length;
  const openLoopsCount = commandQueue.filter(cmd => cmd.type === 'loop-start').length - 
                         commandQueue.filter(cmd => cmd.type === 'loop-end').length;
  const openIfsCount = commandQueue.filter(cmd => cmd.type === 'if-start').length - 
                       commandQueue.filter(cmd => cmd.type === 'if-end').length;
  stats.textContent = `Comandos: ${regularCommands} | Loops: ${openLoopsCount} | IFs: ${openIfsCount} | Duración: ${totalDuration}ms`;
  
  // Verificar bloques abiertos
  showOpenBlocksWarning();
}

// Mover comando arriba
function moveCommandUp(index) {
  if(isPlaying || index <= 0) return;
  
  // Verificar que no se rompan los bloques
  if(!canMoveCommand(index, index - 1)) return;
  
  // Intercambiar elementos
  const temp = commandQueue[index];
  commandQueue[index] = commandQueue[index - 1];
  commandQueue[index - 1] = temp;
  
  renderQueue();
}

// Mover comando abajo
function moveCommandDown(index) {
  if(isPlaying || index >= commandQueue.length - 1) return;
  
  // Verificar que no se rompan los bloques
  if(!canMoveCommand(index, index + 1)) return;
  
  // Intercambiar elementos
  const temp = commandQueue[index];
  commandQueue[index] = commandQueue[index + 1];
  commandQueue[index + 1] = temp;
  
  renderQueue();
}

// Verificar si se puede mover un comando sin romper bloques
function canMoveCommand(fromIndex, toIndex) {
  const item = commandQueue[fromIndex];
  
  // Si es inicio de bloque, no puede moverse después de su fin correspondiente
  if(item.type === 'loop-start' || item.type === 'if-start') {
    const endIndex = findBlockEnd(fromIndex);
    if(endIndex !== -1 && toIndex > endIndex) {
      alert('No se puede mover el inicio del bloque después de su fin correspondiente');
      return false;
    }
  }
  
  // Si es fin de bloque, no puede moverse antes de su inicio correspondiente
  if(item.type === 'loop-end' || item.type === 'if-end') {
    const startIndex = findBlockStart(fromIndex);
    if(startIndex !== -1 && toIndex < startIndex) {
      alert('No se puede mover el fin del bloque antes de su inicio correspondiente');
      return false;
    }
  }
  
  return true;
}

// Encontrar el fin de bloque correspondiente
function findBlockEnd(startIndex) {
  let blockLevel = 0;
  for(let i = startIndex + 1; i < commandQueue.length; i++) {
    if(commandQueue[i].type === 'loop-start' || commandQueue[i].type === 'if-start') {
      blockLevel++;
    } else if(commandQueue[i].type === 'loop-end' || commandQueue[i].type === 'if-end') {
      if(blockLevel === 0) {
        return i;
      } else {
        blockLevel--;
      }
    }
  }
  return -1;
}

// Encontrar el inicio de bloque correspondiente
function findBlockStart(endIndex) {
  let blockLevel = 0;
  for(let i = endIndex - 1; i >= 0; i--) {
    if(commandQueue[i].type === 'loop-end' || commandQueue[i].type === 'if-end') {
      blockLevel++;
    } else if(commandQueue[i].type === 'loop-start' || commandQueue[i].type === 'if-start') {
      if(blockLevel === 0) {
        return i;
      } else {
        blockLevel--;
      }
    }
  }
  return -1;
}

function deleteCommandAt(index) {
  if(isPlaying) return;
  
  const item = commandQueue[index];
  
  // Si es inicio de bloque, eliminar también el fin correspondiente
  if(item.type === 'loop-start' || item.type === 'if-start') {
    const endIndex = findBlockEnd(index);
    if(endIndex !== -1) {
      commandQueue.splice(endIndex, 1);
    }
  }
  // Si es fin de bloque, eliminar también el inicio correspondiente
  else if(item.type === 'loop-end' || item.type === 'if-end') {
    const startIndex = findBlockStart(index);
    if(startIndex !== -1) {
      commandQueue.splice(startIndex, 1);
      // Ajustar índice actual porque removimos un elemento antes
      index--;
    }
  }
  
  commandQueue.splice(index, 1);
  renderQueue();
  blocksAutoClosed = false;
}

function getLabelForType(type) {
  const labels = {
    'motor': 'Control de Motores',
    'servo': 'Servo',
    'pause': 'Pausa',
    'loop-start': 'Loop',
    'loop-end': 'Fin de Loop',
    'if-start': 'IF',
    'if-end': 'Fin de IF'
  };
  return labels[type] || 'Comando';
}

function getInfoForItem(item) {
  if(item.type === 'motor') {
    let info = '';
    if(item.forward && item.forward.length) info += '↑' + item.forward.join(',');
    if(item.backward && item.backward.length) {
      if(info) info += ' ';
      info += '↓' + item.backward.join(',');
    }
    return info + ' | ' + (item.duration || 800) + 'ms';
  } else if(item.type === 'servo') {
    return item.angle + '° por ' + (item.duration || 500) + 'ms';
  } else if(item.type === 'pause') {
    return item.duration + 'ms';
  } else if(item.type === 'loop-start') {
    return 'Repetir ' + (item.repetitions || 2) + ' veces';
  } else if(item.type === 'if-start') {
    return item.info || '';
  } else {
    return '';
  }
}

// Ejecución con soporte para bloques anidados y cierre automático
async function executeQueue() {
  if(isPlaying) return;
  
  // Cerrar bloques automáticamente si es necesario
  const unclosedBlocks = checkOpenBlocks();
  if(unclosedBlocks.length > 0) {
    if(!confirm('Hay ' + unclosedBlocks.join(' y ') + ' sin cerrar. ¿Desea cerrarlos automáticamente antes de ejecutar?')) {
      return;
    }
    autoCloseBlocks();
  }
  
  isPlaying = true;
  currentIndex = 0;
  executedCommands.clear();
  
  await executeFromIndex(0);
  
  stopQueue();
}

async function executeFromIndex(startIndex) {
  let i = startIndex;
  
  while(i < commandQueue.length && isPlaying) {
    currentIndex = i;
    renderQueue();
    
    const item = commandQueue[i];
    
    if(item.type === 'loop-start') {
      // Encontrar el fin del loop correspondiente
      const endIndex = findBlockEnd(i);
      
      if(endIndex !== -1) {
        const repetitions = item.repetitions || 2;
        
        // Ejecutar el contenido del loop
        for(let r = 0; r < repetitions; r++) {
          if(!isPlaying) break;
          
          // Ejecutar comandos dentro del loop
          await executeFromIndex(i + 1);
        }
        
        i = endIndex + 1; // Saltar al después del loop-end
        continue;
      }
    } 
    else if(item.type === 'if-start') {
      // Encontrar el fin del IF correspondiente
      const endIndex = findBlockEnd(i);
      
      if(endIndex !== -1) {
        // Verificar condición
        let conditionMet = false;
        
        if(item.conditionType === 'laps') {
          // Obtener vueltas actuales del servidor
          try {
            const response = await fetch('/status');
            const data = await response.json();
            const laps = data.vueltas || 0;
            
            switch(item.operator) {
              case '>=': conditionMet = laps >= item.value; break;
              case '<=': conditionMet = laps <= item.value; break;
              case '==': conditionMet = laps == item.value; break;
              case '>': conditionMet = laps > item.value; break;
              case '<': conditionMet = laps < item.value; break;
            }
          } catch(e) {
            console.error('Error obteniendo vueltas:', e);
          }
        } else if(item.conditionType === 'sensor') {
          // Obtener estado del sensor del servidor
          try {
            const response = await fetch('/sensor');
            const data = await response.json();
            const sensorState = data.sensor === 'HIGH';
            
            if(item.sensorState === 'HIGH') {
              conditionMet = sensorState;
            } else {
              conditionMet = !sensorState;
            }
          } catch(e) {
            console.error('Error obteniendo estado del sensor:', e);
          }
        }
        
        // Si la condición se cumple, ejecutar el bloque
        if(conditionMet) {
          await executeFromIndex(i + 1);
        }
        
        i = endIndex + 1; // Saltar al después del if-end
        continue;
      }
    }
    else if(item.type === 'loop-end' || item.type === 'if-end') {
      // Fin de bloque - retornar al llamador
      return;
    }
    else {
      // Comando normal
      executedCommands.add(item.id);
      await executeCommand(item);
      i++;
    }
  }
}

async function executeCommand(item) {
  console.log(`Ejecutando: ${item.cmd} por ${item.duration || 800}ms`);
  
  // Enviar comando al ESP32
  if(item.type === 'pause') {
    // Solo esperar para pausas
    await new Promise(resolve => setTimeout(resolve, item.duration || 1000));
  } else {
    fetch('/control?cmd=' + encodeURIComponent(item.cmd));
    await new Promise(resolve => setTimeout(resolve, item.duration || 800));
    
    // Solo enviar stop si no es el último comando
    const nextIndex = currentIndex + 1;
    if(nextIndex < commandQueue.length) {
      const nextItem = commandQueue[nextIndex];
      if(nextItem.type !== 'loop-end' && nextItem.type !== 'if-end' && nextItem.type !== 'pause') {
        fetch('/control?cmd=stop');
        await new Promise(resolve => setTimeout(resolve, 50));
      }
    }
  }
}

async function playQueue() {
  if(commandQueue.length === 0 || isPlaying) return;
  
  await executeQueue();
}

function stopQueue() {
  isPlaying = false;
  currentIndex = 0;
  executedCommands.clear();
  fetch('/control?cmd=stop');
  renderQueue();
}

function pauseQueue() {
  isPlaying = false;
  fetch('/control?cmd=stop');
}

function clearQueue() {
  stopQueue();
  commandQueue = [];
  blocksAutoClosed = false;
  renderQueue();
}

// UI
function showTab(name) {
  document.getElementById('tab-manual').style.display = name === 'manual' ? 'block' : 'none';
  document.getElementById('tab-program').style.display = name === 'program' ? 'block' : 'none';
  document.querySelectorAll('.tab').forEach((b, i) => {
    b.classList.toggle('active', (name === 'manual' && i === 0) || (name === 'program' && i === 1));
  });
  
  if(name === 'program') {
    renderQueue();
  }
}

function closeWelcome() {
  document.getElementById('welcomeModal').style.display = 'none';
}

function resetLaps() {
  fetch('/reset', {method:'POST'});
  document.getElementById('mensaje').textContent = 'Reiniciado';
  setTimeout(() => document.getElementById('mensaje').textContent = '', 1500);
}

function openMotorModal() {
  document.getElementById('motorModal').classList.remove('hidden');
}

function closeMotorModal() {
  document.getElementById('motorModal').classList.add('hidden');
}

function openServoModal() {
  document.getElementById('servoModal').classList.remove('hidden');
}

function closeServoModal() {
  document.getElementById('servoModal').classList.add('hidden');
}

function closePauseModal() {
  document.getElementById('pauseModal').classList.add('hidden');
}

function closeLoopModal() {
  document.getElementById('loopModal').classList.add('hidden');
}

function closeIfModal() {
  document.getElementById('ifModal').classList.add('hidden');
}

// Status updates
setInterval(() => {
  fetch('/status').then(r => r.json()).then(data => {
    document.getElementById('vueltas').textContent = data.vueltas;
    document.getElementById('mensaje').textContent = data.mensaje;
  });
}, 500);

// Inicialización
document.addEventListener('DOMContentLoaded', () => {
  renderQueue();
});
</script>
</body>
</html>
)rawliteral";

// Funciones de movimiento
void setMotor(int index, bool forward) {
  digitalWrite(motorPins[index*2], forward ? HIGH : LOW);
  digitalWrite(motorPins[index*2+1], forward ? LOW : HIGH);
}

void stopMotor(int index) {
  digitalWrite(motorPins[index*2], LOW);
  digitalWrite(motorPins[index*2+1], LOW);
  motorStates[index] = false;
}

void stopAllMotors() {
  for(int i = 0; i < 8; i++) {
    digitalWrite(motorPins[i], LOW);
  }
  for(int i = 0; i < 4; i++) {
    motorStates[i] = false;
  }
}

void controlMotor(int motorNum, String command) {
  motorNum--; // 0-3
  if(motorNum < 0 || motorNum > 3) return;
  
  if(command == "forward") {
    setMotor(motorNum, true);
    motorStates[motorNum] = true;
    motorDirections[motorNum] = true;
  } 
  else if(command == "backward") {
    setMotor(motorNum, false);
    motorStates[motorNum] = true;
    motorDirections[motorNum] = false;
  }
  else if(command == "stop") {
    stopMotor(motorNum);
  }
  
  Serial.printf("Motor %d: %s\n", motorNum+1, command.c_str());
}

void controlMixedMotors(String command) {
  Serial.printf("Mixed: %s\n", command.c_str());
  
  // Extraer tiempo
  int timeIndex = command.indexOf("_time");
  unsigned long duration = 800;
  
  if(timeIndex != -1) {
    String timeStr = command.substring(timeIndex + 5);
    duration = timeStr.toInt();
    command = command.substring(0, timeIndex);
  }
  
  // Procesar forward
  int fIndex = command.indexOf("forward");
  if(fIndex != -1) {
    int start = fIndex + 7;
    int end = command.indexOf("backward");
    if(end == -1) end = command.length();
    
    String motors = command.substring(start, end);
    if(motors.endsWith("_")) motors = motors.substring(0, motors.length()-1);
    
    int last = -1;
    for(int i = 0; i <= motors.length(); i++) {
      if(i == motors.length() || motors.charAt(i) == ',') {
        if(last + 1 < i) {
          String num = motors.substring(last + 1, i);
          controlMotor(num.toInt(), "forward");
        }
        last = i;
      }
    }
  }
  
  // Procesar backward
  int bIndex = command.indexOf("backward");
  if(bIndex != -1) {
    int start = bIndex + 8;
    String motors = command.substring(start);
    
    int last = -1;
    for(int i = 0; i <= motors.length(); i++) {
      if(i == motors.length() || motors.charAt(i) == ',') {
        if(last + 1 < i) {
          String num = motors.substring(last + 1, i);
          controlMotor(num.toInt(), "backward");
        }
        last = i;
      }
    }
  }
  
  delay(duration);
  stopAllMotors();
}

// Función para evaluar condiciones IF
bool evaluarCondicion(String cmd) {
  if(cmd.startsWith("if_laps_")) {
    // Formato: if_laps_OPERADOR_VALOR
    int us1 = cmd.indexOf('_', 7);
    int us2 = cmd.indexOf('_', us1+1);
    
    if(us1 != -1 && us2 != -1) {
      String operador = cmd.substring(us1+1, us2);
      int valor = cmd.substring(us2+1).toInt();
      
      Serial.printf("Evaluando condición LAPS: %s %d, actual: %lu\n", 
                   operador.c_str(), valor, contadorVueltas);
      
      if(operador == ">=") return contadorVueltas >= valor;
      if(operador == "<=") return contadorVueltas <= valor;
      if(operador == "==") return contadorVueltas == valor;
      if(operador == ">") return contadorVueltas > valor;
      if(operador == "<") return contadorVueltas < valor;
    }
  }
  else if(cmd.startsWith("if_sensor_")) {
    // Formato: if_sensor_HIGH/LOW
    String estado = cmd.substring(10);
    bool sensorActual = digitalRead(pinSensorLinea);
    
    Serial.printf("Evaluando condición SENSOR: %s, actual: %s\n", 
                 estado.c_str(), sensorActual ? "HIGH" : "LOW");
    
    if(estado == "HIGH") return sensorActual == HIGH;
    if(estado == "LOW") return sensorActual == LOW;
  }
  
  return false;
}

void processCommand(String cmd) {
  Serial.println("Processing: " + cmd);
  
  if(cmd == "left") {
    direccion.write(55);
    delay(80);
  }
  else if(cmd == "right") {
    direccion.write(125);
    delay(80);
  }
  else if(cmd == "stop") {
    stopAllMotors();
  }
  else if(cmd.startsWith("mixedmotor_")) {
    controlMixedMotors(cmd);
  }
  else if(cmd.startsWith("motor")) {
    // Formato: motor_X_forward/backward
    int us1 = cmd.indexOf('_');
    int us2 = cmd.indexOf('_', us1+1);
    if(us1 != -1 && us2 != -1) {
      int num = cmd.substring(us1+1, us2).toInt();
      String dir = cmd.substring(us2+1);
      controlMotor(num, dir);
    }
  }
  else if(cmd.startsWith("servo_")) {
    // Formato: servo_ANGULO_timeDURACION
    int us1 = cmd.indexOf('_');
    int us2 = cmd.indexOf('_', us1+1);
    int timeIndex = cmd.indexOf("time");
    
    if(us1 != -1 && us2 != -1) {
      int angle = cmd.substring(us1+1, us2).toInt();
      unsigned long duration = 500;
      
      if(timeIndex != -1) {
        duration = cmd.substring(timeIndex + 4).toInt();
      }
      
      // Limitar ángulo a rango seguro
      if(angle < 55) angle = 55;
      if(angle > 125) angle = 125;
      
      direccion.write(angle);
      delay(duration);
      Serial.printf("Servo: %d degrees for %lu ms\n", angle, duration);
    }
  }
  else if(cmd.startsWith("pause_")) {
    // Formato: pause_DURACION
    int us1 = cmd.indexOf('_');
    if(us1 != -1) {
      unsigned long duration = cmd.substring(us1 + 1).toInt();
      delay(duration);
      Serial.printf("Pausa: %lu ms\n", duration);
    }
  }
  else if(cmd.startsWith("loop_start_")) {
    // Formato: loop_start_REPETICIONES
    int us1 = cmd.indexOf('_', 5);
    if(us1 != -1) {
      int repeticiones = cmd.substring(us1 + 1).toInt();
      Serial.printf("Loop start: %d repeticiones\n", repeticiones);
    }
  }
  else if(cmd.startsWith("loop_end")) {
    Serial.println("Loop end command");
  }
  else if(cmd.startsWith("if_laps_") || cmd.startsWith("if_sensor_")) {
    bool resultado = evaluarCondicion(cmd);
    Serial.printf("Condición IF: %s -> %s\n", cmd.c_str(), resultado ? "VERDADERO" : "FALSO");
    condicionCumplida = resultado;
    condicionIfActiva = true;
  }
  else if(cmd == "if_end") {
    condicionIfActiva = false;
    Serial.println("IF end command");
  }
}

// Configurar WiFi optimizada para ESP32-C3
void configurarWiFi() {
  String ssid = "BRINKO-" + String(ESP.getEfuseMac() >> 32, HEX);
  
  Serial.println("Configurando WiFi para ESP32-C3...");
  Serial.print("SSID: ");
  Serial.println(ssid);
  
  // Configurar modo Access Point (compatible con ESP32-C3)
  WiFi.mode(WIFI_AP);
  
  // Configurar IP estática
  IPAddress local_ip(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  if (!WiFi.softAPConfig(local_ip, gateway, subnet)) {
    Serial.println("Error en configuración de red");
    return;
  }
  
  // Iniciar Access Point con parámetros básicos pero optimizados
  bool apStarted = WiFi.softAP(ssid.c_str(), password, WIFI_CHANNEL, 0, MAX_CLIENTS);
  
  if (!apStarted) {
    Serial.println("Error al iniciar Access Point");
    // Intentar sin parámetros avanzados
    apStarted = WiFi.softAP(ssid.c_str(), password);
    
    if (!apStarted) {
      Serial.println("Error crítico: No se pudo iniciar Access Point");
      return;
    }
  }
  
  // Configuraciones adicionales compatibles con ESP32-C3
  WiFi.setSleep(false); // Desactivar sleep para mejor estabilidad
  
  // Para ESP32-C3, usar métodos básicos de la librería WiFi
  // Los métodos avanzados (esp_wifi_*) no están disponibles en esta versión
  
  Serial.println("=========================================");
  Serial.print("Access Point iniciado: ");
  Serial.println(ssid);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("Dirección MAC: ");
  Serial.println(WiFi.softAPmacAddress());
  Serial.print("Número de clientes máx: ");
  Serial.println(MAX_CLIENTS);
  Serial.print("Canal WiFi: ");
  Serial.println(WIFI_CHANNEL);
  Serial.println("=========================================");
  Serial.println("WiFi listo para conexiones");
}

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n=== BRINKO Motora ESP32-C3 Iniciando ===\n");
  
  // Configurar pines
  for(int i = 0; i < 8; i++) {
    pinMode(motorPins[i], OUTPUT);
    digitalWrite(motorPins[i], LOW);
  }
  
  pinMode(pinSensorLinea, INPUT_PULLUP);
  
  // Servo
  direccion.attach(pinServo);
  direccion.write(90);
  delay(500);
  
  // Configurar WiFi optimizado para ESP32-C3
  configurarWiFi();
  
  // Configurar servidor web
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", html);
  });
  
  server.on("/control", HTTP_GET, []() {
    if(server.hasArg("cmd")) {
      processCommand(server.arg("cmd"));
    }
    else if(server.hasArg("x") && server.hasArg("y")) {
      int x = server.arg("x").toInt();
      int y = server.arg("y").toInt();
      
      // Servo
      int angle = map(x, -100, 100, 125, 55);
      direccion.write(angle);
      
      // Motores
      if(y > 30) {
        for(int i = 0; i < 4; i++) {
          setMotor(i, true);
          motorStates[i] = true;
          motorDirections[i] = true;
        }
      }
      else if(y < -30) {
        for(int i = 0; i < 4; i++) {
          setMotor(i, false);
          motorStates[i] = true;
          motorDirections[i] = false;
        }
      }
      else {
        stopAllMotors();
      }
    }
    server.send(200, "text/plain", "OK");
  });
  
  server.on("/motor", HTTP_GET, []() {
    if(server.hasArg("num") && server.hasArg("dir")) {
      int num = server.arg("num").toInt();
      String dir = server.arg("dir");
      controlMotor(num, dir);
    }
    server.send(200, "text/plain", "OK");
  });
  
  server.on("/servo", HTTP_GET, []() {
    if(server.hasArg("angle")) {
      int angle = server.arg("angle").toInt();
      // Limitar ángulo a rango seguro
      if(angle < 55) angle = 55;
      if(angle > 125) angle = 125;
      
      direccion.write(angle);
      Serial.printf("Servo manual: %d degrees\n", angle);
    }
    server.send(200, "text/plain", "OK");
  });
  
  server.on("/status", HTTP_GET, []() {
    String json = "{\"vueltas\":" + String(contadorVueltas) + ",\"mensaje\":\"" + mensaje + "\"}";
    server.send(200, "application/json", json);
  });
  
  server.on("/sensor", HTTP_GET, []() {
    bool estado = digitalRead(pinSensorLinea);
    String json = "{\"sensor\":\"" + String(estado ? "HIGH" : "LOW") + "\"}";
    server.send(200, "application/json", json);
  });
  
  server.on("/reset", HTTP_POST, []() {
    contadorVueltas = 0;
    mensaje = "Reinicio";
    server.send(200, "text/plain", "OK");
  });
  
  server.begin();
  Serial.println("Servidor web listo en puerto 80");
  Serial.println("\n=== Sistema Listo ===\n");
  Serial.println("INSTRUCCIONES:");
  Serial.println("1. Conéctate a la red WiFi: BRINKO-XXXX");
  Serial.println("2. Contraseña: brinko100");
  Serial.println("3. Abre el navegador en: http://192.168.4.1");
  Serial.println("=========================================\n");
}

void loop() {
  server.handleClient();
  
  // Sensor de línea
  bool lectura = digitalRead(pinSensorLinea);
  sensorLineaEstado = lectura;
  
  if(lectura != ultimoEstado) {
    ultimoEstado = lectura;
    if(lectura == LOW) {
      contadorVueltas++;
      Serial.println("Lap: " + String(contadorVueltas));
    }
  }
  
  // Meta
  if(contadorVueltas == 5 && mensaje == "") {
    mensaje = "Meta!";
    tiempoMensaje = millis();
    contadorVueltas = 0;
  }
  
  if(mensaje != "" && millis() - tiempoMensaje > 3000) {
    mensaje = "";
  }
  
  // Monitoreo de conexiones WiFi
  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck > 15000) { // Cada 15 segundos
    lastWiFiCheck = millis();
    int clients = WiFi.softAPgetStationNum();
    if (clients > 0) {
      Serial.printf("Clientes WiFi conectados: %d\n", clients);
    } else {
      Serial.println("Esperando conexiones WiFi...");
    }
  }
  
  delay(10);
}