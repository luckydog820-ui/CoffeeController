#ifndef COFFEE_CONTROLLER_WEB_APP_H_
#define COFFEE_CONTROLLER_WEB_APP_H_

#include <Arduino.h>

static const char kIndexHtml[] PROGMEM = R"COFFEEHTML(<!doctype html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#17110e">
<title>CoffeePilot · ESP32-S3</title>
<style>
:root{color-scheme:dark;--bg:#17110e;--panel:#211814;--panel2:#2a1e18;--line:rgba(255,235,213,.12);--text:#fff6eb;--muted:#ad9d90;--crema:#e8b76f;--amber:#f0a451;--good:#79c8a1;--bad:#fa7c72;--blue:#78b7d9;--shadow:0 22px 60px rgba(0,0,0,.28)}
*{box-sizing:border-box}
html{background:var(--bg)}
body{margin:0;min-height:100svh;background:radial-gradient(circle at 84% -10%,rgba(208,130,55,.22),transparent 34rem),radial-gradient(circle at -12% 38%,rgba(114,66,38,.18),transparent 28rem),linear-gradient(155deg,#17110e,#120e0c 70%);color:var(--text);font-family:ui-sans-serif,-apple-system,BlinkMacSystemFont,"Segoe UI","PingFang SC","Microsoft YaHei",sans-serif;-webkit-font-smoothing:antialiased}
button,input{font:inherit}
button{-webkit-tap-highlight-color:transparent}
.shell{width:min(100%,760px);margin:0 auto;padding:max(18px,env(safe-area-inset-top)) 16px calc(108px + env(safe-area-inset-bottom))}
.topbar{display:flex;align-items:center;justify-content:space-between;gap:12px;margin-bottom:22px}
.brand{display:flex;align-items:center;gap:11px;min-width:0}
.mark{width:38px;height:38px;border-radius:13px;background:linear-gradient(145deg,var(--crema),#a95f2d);display:grid;place-items:center;color:#25150c;box-shadow:0 8px 24px rgba(218,146,75,.24)}
.mark svg{width:22px;height:22px}
.brand h1{font-size:17px;letter-spacing:.02em;margin:0;line-height:1.1}
.brand p{font-size:11px;color:var(--muted);margin:4px 0 0;letter-spacing:.08em;text-transform:uppercase}
.connection{display:flex;align-items:center;gap:7px;border:1px solid var(--line);background:rgba(33,24,20,.76);padding:9px 12px;border-radius:999px;color:var(--muted);font-size:12px;white-space:nowrap}
.connection::before{content:"";width:7px;height:7px;border-radius:50%;background:var(--good);box-shadow:0 0 0 4px rgba(121,200,161,.1)}
.connection.offline::before{background:var(--bad);box-shadow:0 0 0 4px rgba(250,124,114,.1)}
.view{animation:in .24s ease-out}
.view[hidden]{display:none}
@keyframes in{from{opacity:0;transform:translateY(5px)}to{opacity:1;transform:none}}
.status-line{display:flex;align-items:center;gap:9px;margin:0 2px 12px;color:var(--muted);font-size:13px}
.state-dot{width:8px;height:8px;border-radius:50%;background:var(--muted);flex:none}
.state-dot.ready{background:var(--good);box-shadow:0 0 13px rgba(121,200,161,.55)}
.state-dot.active{background:var(--amber);box-shadow:0 0 13px rgba(240,164,81,.7);animation:pulse 1.45s infinite}
.state-dot.fault{background:var(--bad);box-shadow:0 0 14px rgba(250,124,114,.7);animation:pulse .9s infinite}
@keyframes pulse{50%{opacity:.42;transform:scale(.76)}}
.state-name{color:var(--text);font-weight:650}
.fault-banner{display:none;margin:0 0 14px;padding:13px 15px;border:1px solid rgba(250,124,114,.35);background:rgba(91,32,29,.38);border-radius:16px;color:#ffc2bc;font-size:13px;line-height:1.5}
.fault-banner.show{display:block}
.live-grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.metric-card,.chart-card,.panel-card{background:linear-gradient(150deg,rgba(43,31,25,.96),rgba(31,23,19,.96));border:1px solid var(--line);border-radius:21px;box-shadow:var(--shadow)}
.metric-card{min-height:148px;padding:17px 16px 15px;position:relative;overflow:hidden}
.metric-card::after{content:"";position:absolute;width:100px;height:100px;right:-48px;top:-48px;border-radius:50%;background:rgba(232,183,111,.08)}
.eyebrow{font-size:11px;letter-spacing:.1em;color:var(--muted);text-transform:uppercase;margin:0 0 13px}
.reading{display:flex;align-items:baseline;gap:5px;line-height:1}
.reading strong{font-size:clamp(37px,10vw,52px);font-weight:580;letter-spacing:-.055em;font-variant-numeric:tabular-nums}
.reading span{font-size:13px;color:var(--muted)}
.micro-row{display:flex;justify-content:space-between;gap:8px;margin-top:15px;color:var(--muted);font-size:11px}
.meter{height:4px;background:rgba(255,255,255,.07);border-radius:9px;overflow:hidden;margin-top:11px}
.meter i{display:block;height:100%;width:0;background:linear-gradient(90deg,#b96d39,var(--crema));border-radius:inherit;transition:width .5s ease}
.meter.pressure i{background:linear-gradient(90deg,#a7652f,var(--amber))}
.brew-zone{display:flex;align-items:center;justify-content:space-between;gap:16px;margin:13px 0 12px;padding:14px 8px 14px 17px;background:linear-gradient(135deg,rgba(233,181,108,.13),rgba(38,27,22,.88));border:1px solid rgba(232,183,111,.19);border-radius:22px}
.brew-copy strong{display:block;font-size:15px;font-weight:620}
.brew-copy span{display:block;color:var(--muted);font-size:12px;line-height:1.45;margin-top:5px;max-width:210px}
.brew-button{width:92px;height:92px;flex:none;border:0;border-radius:50%;background:linear-gradient(145deg,#f4c781,#db9148 64%,#aa602b);color:#2d180b;box-shadow:0 13px 33px rgba(210,130,56,.3),inset 0 1px 0 rgba(255,255,255,.58);display:grid;place-items:center;cursor:pointer;transition:transform .16s,filter .16s;font-weight:760;font-size:15px;touch-action:manipulation}
.brew-button:active{transform:scale(.96)}
.brew-button.stop{background:linear-gradient(145deg,#ff9b8d,#d9584e);color:#2b0d0a;box-shadow:0 13px 33px rgba(214,73,63,.28)}
.brew-button:disabled{filter:grayscale(.75);opacity:.45;box-shadow:none;cursor:not-allowed}
.brew-button svg{width:25px;height:25px;display:block;margin:0 auto 3px}
.chart-card{padding:16px 15px 11px;margin-bottom:12px}
.card-head{display:flex;align-items:center;justify-content:space-between;gap:12px;margin-bottom:12px}
.card-head h2,.section-title{font-size:14px;font-weight:620;margin:0}
.legend{display:flex;gap:12px;font-size:10px;color:var(--muted)}
.legend i{display:inline-block;width:6px;height:6px;border-radius:50%;margin-right:5px;vertical-align:1px}
.legend .p i{background:var(--crema)}.legend .t i{background:var(--blue)}
.chart-wrap{height:126px;position:relative}
canvas{width:100%;height:100%;display:block}
.mini-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}
.mini{padding:13px 12px;border-radius:16px;background:rgba(35,25,21,.92);border:1px solid var(--line)}
.mini span{display:block;color:var(--muted);font-size:10px;margin-bottom:6px}
.mini strong{font-size:16px;font-weight:600;font-variant-numeric:tabular-nums}
.section-head{padding:3px 2px 17px}
.section-head h2{font-size:24px;line-height:1.1;margin:0;font-weight:610;letter-spacing:-.025em}
.section-head p{color:var(--muted);font-size:12px;line-height:1.5;margin:7px 0 0}
.panel-card{padding:5px 17px;margin-bottom:12px;box-shadow:none}
.setting{padding:16px 0;border-bottom:1px solid var(--line)}
.setting:last-child{border:0}
.setting-label{display:flex;align-items:center;justify-content:space-between;gap:12px}
.setting-label label,.setting-label span{font-size:13px;color:#f5e8db}
.setting-label output{font-size:13px;color:var(--crema);font-variant-numeric:tabular-nums}
.setting small{display:block;color:var(--muted);font-size:11px;line-height:1.45;margin-top:5px}
input[type=range]{appearance:none;width:100%;height:24px;background:transparent;margin:10px 0 0;touch-action:pan-y}
input[type=range]::-webkit-slider-runnable-track{height:5px;border-radius:9px;background:linear-gradient(90deg,var(--crema) 0 var(--fill,50%),rgba(255,255,255,.09) var(--fill,50%) 100%)}
input[type=range]::-webkit-slider-thumb{appearance:none;width:22px;height:22px;border-radius:50%;background:#fff4e7;border:4px solid #c68143;margin-top:-8px;box-shadow:0 4px 14px rgba(0,0,0,.35)}
input[type=range]::-moz-range-track{height:5px;border-radius:9px;background:rgba(255,255,255,.09)}
input[type=range]::-moz-range-progress{height:5px;border-radius:9px;background:var(--crema)}
input[type=range]::-moz-range-thumb{width:16px;height:16px;border-radius:50%;background:#fff4e7;border:3px solid #c68143}
.save-row{display:flex;align-items:center;gap:12px;justify-content:flex-end;margin:17px 0 7px}
.save-note{color:var(--muted);font-size:11px;margin-right:auto}
.primary,.secondary,.danger{border:0;border-radius:14px;min-height:48px;padding:0 20px;font-weight:660;cursor:pointer;touch-action:manipulation}
.primary{background:linear-gradient(145deg,var(--crema),#d78e48);color:#2b180b;box-shadow:0 9px 22px rgba(211,137,67,.2)}
.primary:disabled{opacity:.5}
.secondary{background:rgba(255,255,255,.07);color:var(--text);border:1px solid var(--line)}
.danger{background:rgba(250,124,114,.13);color:#ffb7b0;border:1px solid rgba(250,124,114,.25)}
.system-row{display:flex;align-items:center;justify-content:space-between;gap:14px;padding:15px 0;border-bottom:1px solid var(--line);font-size:13px}
.system-row:last-child{border:0}
.system-row span{color:var(--muted);font-size:11px;display:block;margin-top:4px}
.pill{font-size:11px;padding:6px 9px;border-radius:999px;background:rgba(121,200,161,.1);color:var(--good);white-space:nowrap}
.pill.bad{background:rgba(250,124,114,.1);color:var(--bad)}
.hint{color:var(--muted);font-size:11px;line-height:1.65;padding:1px 4px 12px}
.nav{position:fixed;z-index:20;left:50%;bottom:calc(10px + env(safe-area-inset-bottom));transform:translateX(-50%);width:min(calc(100% - 24px),520px);padding:7px;border:1px solid rgba(255,235,213,.13);border-radius:20px;background:rgba(26,19,16,.9);backdrop-filter:blur(18px);display:grid;grid-template-columns:repeat(3,1fr);box-shadow:0 18px 50px rgba(0,0,0,.45)}
.nav button{border:0;background:transparent;color:var(--muted);min-height:50px;border-radius:14px;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:3px;font-size:10px;cursor:pointer}
.nav button.active{background:rgba(232,183,111,.12);color:var(--crema)}
.nav svg{width:19px;height:19px}
.toast{position:fixed;z-index:40;left:50%;bottom:calc(88px + env(safe-area-inset-bottom));transform:translate(-50%,16px);opacity:0;pointer-events:none;background:#f5e7d7;color:#2a190f;padding:11px 16px;border-radius:999px;box-shadow:0 14px 38px rgba(0,0,0,.35);font-size:12px;font-weight:650;transition:.22s;max-width:calc(100% - 40px);text-align:center}
.toast.show{opacity:1;transform:translate(-50%,0)}
dialog{border:0;padding:0;background:transparent;color:var(--text);max-width:calc(100% - 36px)}
dialog::backdrop{background:rgba(8,5,4,.68);backdrop-filter:blur(7px)}
.dialog-box{width:min(360px,calc(100vw - 36px));padding:22px;border-radius:22px;background:#261b16;border:1px solid var(--line);box-shadow:var(--shadow)}
.dialog-box h3{margin:0 0 8px;font-size:17px}
.dialog-box p{margin:0;color:var(--muted);font-size:13px;line-height:1.6}
.dialog-actions{display:flex;gap:9px;margin-top:20px}
.dialog-actions button{flex:1}
@media(min-width:700px){
 .shell{padding-top:32px}
 .overview-grid{display:grid;grid-template-columns:.92fr 1.08fr;gap:12px;align-items:start}
 .overview-right .brew-zone{margin-top:0}
 .nav{position:sticky;bottom:18px;margin-top:26px}
}
@media(prefers-reduced-motion:reduce){*{animation:none!important;transition:none!important}}
</style>
</head>
<body>
<div class="shell">
  <header class="topbar">
    <div class="brand">
      <div class="mark" aria-hidden="true">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><path d="M5 8h11v6.5A4.5 4.5 0 0 1 11.5 19h-2A4.5 4.5 0 0 1 5 14.5V8Z"/><path d="M16 10h1.5a2.5 2.5 0 0 1 0 5H16M8 5c0-1 1-1 1-2m4 2c0-1 1-1 1-2"/></svg>
      </div>
      <div><h1>CoffeePilot</h1><p>ESP32-S3 Controller</p></div>
    </div>
    <div class="connection" id="connection">连接中</div>
  </header>

  <main>
    <section class="view" data-view="overview">
      <div class="status-line">
        <i class="state-dot" id="stateDot"></i>
        <span class="state-name" id="stateName">正在连接</span>
        <span id="stateHint">等待控制器状态</span>
      </div>
      <div class="fault-banner" id="faultBanner"></div>

      <div class="overview-grid">
        <div class="overview-left">
          <div class="live-grid">
            <article class="metric-card">
              <p class="eyebrow">锅炉温度</p>
              <div class="reading"><strong id="tempValue">--</strong><span>°C</span></div>
              <div class="meter"><i id="tempMeter"></i></div>
              <div class="micro-row"><span>目标</span><span><b id="tempTarget">--</b> °C</span></div>
            </article>
            <article class="metric-card">
              <p class="eyebrow">当前压力</p>
              <div class="reading"><strong id="pressureValue">--</strong><span>bar</span></div>
              <div class="meter pressure"><i id="pressureMeter"></i></div>
              <div class="micro-row"><span>泵输出</span><span><b id="pumpVoltageInline">--</b> V</span></div>
            </article>
          </div>
        </div>

        <div class="overview-right">
          <div class="brew-zone">
            <div class="brew-copy">
              <strong id="brewTitle">自动萃取</strong>
              <span id="brewDescription">预热完成后即可开始一杯</span>
            </div>
            <button class="brew-button" id="brewButton" disabled aria-label="开始萃取">
              <span><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 3v8m0 0 3-3m-3 3-3-3"/><path d="M5 14c1.8 3 4.1 4.5 7 4.5s5.2-1.5 7-4.5M4 21h16"/></svg><span id="brewButtonText">开始</span></span>
            </button>
          </div>

          <article class="chart-card">
            <div class="card-head">
              <h2>实时曲线 · 90 秒</h2>
              <div class="legend"><span class="p"><i></i>压力</span><span class="t"><i></i>温度</span></div>
            </div>
            <div class="chart-wrap"><canvas id="liveChart" aria-label="压力和温度实时曲线"></canvas></div>
          </article>

          <div class="mini-grid">
            <div class="mini"><span>泵控制源</span><strong id="pumpSource">--</strong></div>
            <div class="mini"><span>本杯时间</span><strong id="brewElapsed">0.0s</strong></div>
            <div class="mini"><span>通信</span><strong id="commsState">--</strong></div>
          </div>
        </div>
      </div>
    </section>

    <section class="view" data-view="recipe" hidden>
      <div class="section-head"><h2>萃取配方</h2><p>先调整目标，再开始萃取。运行中禁止保存，避免中途改变安全边界。</p></div>
      <div class="panel-card">
        <div class="setting">
          <div class="setting-label"><label for="preVoltage">预浸泡泵电压</label><output id="preVoltageOut"></output></div>
          <input type="range" id="preVoltage" min="0" max="3" step="0.05">
          <small>低压湿润粉饼，减少通道效应。</small>
        </div>
        <div class="setting">
          <div class="setting-label"><label for="preTime">预浸泡时间</label><output id="preTimeOut"></output></div>
          <input type="range" id="preTime" min="0" max="10" step="0.5">
        </div>
        <div class="setting">
          <div class="setting-label"><label for="extractionPressure">主萃取压力</label><output id="extractionPressureOut"></output></div>
          <input type="range" id="extractionPressure" min="3" max="12" step="0.1">
        </div>
        <div class="setting">
          <div class="setting-label"><label for="extractionTime">最长萃取时间</label><output id="extractionTimeOut"></output></div>
          <input type="range" id="extractionTime" min="5" max="45" step="1">
        </div>
        <div class="setting">
          <div class="setting-label"><label for="maxPressure">硬压力上限</label><output id="maxPressureOut"></output></div>
          <input type="range" id="maxPressure" min="5" max="13.5" step="0.1">
          <small>软件强制断电阈值；硬件仍应保留机械/电气保护。</small>
        </div>
      </div>
      <div class="save-row"><span class="save-note">参数保存在设备 NVS</span><button class="primary" id="saveRecipe">保存配方</button></div>
    </section>

    <section class="view" data-view="system" hidden>
      <div class="section-head"><h2>设备设置</h2><p>温度由 E5CC 闭环，ESP32-S3 只写入设定值并监控回读。</p></div>
      <div class="panel-card">
        <div class="setting">
          <div class="setting-label"><label for="boilerSp">锅炉目标温度</label><output id="boilerSpOut"></output></div>
          <input type="range" id="boilerSp" min="70" max="100" step="0.5">
        </div>
        <div class="setting">
          <div class="setting-label"><label for="pidKp">压力环 Kp</label><output id="pidKpOut"></output></div>
          <input type="range" id="pidKp" min="0" max="3" step="0.05">
        </div>
        <div class="setting">
          <div class="setting-label"><label for="pidKi">压力环 Ki</label><output id="pidKiOut"></output></div>
          <input type="range" id="pidKi" min="0" max="3" step="0.05">
        </div>
        <div class="setting">
          <div class="setting-label"><label for="pidKd">压力环 Kd</label><output id="pidKdOut"></output></div>
          <input type="range" id="pidKd" min="0" max="1" step="0.01">
          <small>建议从 PI 开始整定，压力曲线稳定后再启用 D。</small>
        </div>
      </div>
      <div class="save-row"><span class="save-note">修改后需重新达到目标温度</span><button class="primary" id="saveSystem">保存设置</button></div>

      <div class="panel-card" style="margin-top:16px">
        <div class="system-row"><div>E5CC Modbus<span id="modbusDetail">等待状态</span></div><b class="pill" id="modbusPill">--</b></div>
        <div class="system-row"><div>压力传感器<span id="sensorDetail">等待状态</span></div><b class="pill" id="sensorPill">--</b></div>
        <div class="system-row"><div>网络地址<span id="ipDetail">--</span></div><b class="pill" id="wifiPill">Wi-Fi</b></div>
        <div class="system-row"><div>设备热点<span id="apDetail">--</span></div><b class="pill" id="apPill">--</b></div>
        <div class="system-row"><div>硬件型号<span id="boardDetail">--</span></div><span id="firmwareVersion">--</span></div>
        <div class="system-row"><div>空闲内存<span id="heapDetail">--</span></div><span>ESP32-S3</span></div>
      </div>
      <button class="danger" id="resetFault" style="width:100%;margin-top:2px">安全故障复位</button>
      <p class="hint">复位前确认压力已释放、温度低于上限、旋钮在关断位且 E5CC 通信恢复。故障复位不能替代硬件急停。</p>
    </section>
  </main>

  <nav class="nav" aria-label="主导航">
    <button class="active" data-tab="overview"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><path d="M4 13h6V4H4v9Zm10 7h6v-9h-6v9ZM4 20h6v-3H4v3Zm10-13h6V4h-6v3Z"/></svg>总览</button>
    <button data-tab="recipe"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><path d="M7 4h10v16H7zM9.5 8h5M9.5 12h5M9.5 16h3"/></svg>配方</button>
    <button data-tab="system"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.7 1.7 0 0 0 .34 1.88l.06.06-2.86 2.86-.06-.06A1.7 1.7 0 0 0 15 19.4a1.7 1.7 0 0 0-1 .6 1.7 1.7 0 0 0-.4 1.1V21H9.6v-.1A1.7 1.7 0 0 0 8.5 19.4a1.7 1.7 0 0 0-1.88.34l-.06.06-2.86-2.86.06-.06A1.7 1.7 0 0 0 4.1 15a1.7 1.7 0 0 0-.6-1 1.7 1.7 0 0 0-1.1-.4H2V9.6h.4A1.7 1.7 0 0 0 4.1 8.5a1.7 1.7 0 0 0-.34-1.88l-.06-.06L6.56 3.7l.06.06A1.7 1.7 0 0 0 8.5 4.1a1.7 1.7 0 0 0 1-.6 1.7 1.7 0 0 0 .4-1.1V2h4v.4a1.7 1.7 0 0 0 1.1 1.5 1.7 1.7 0 0 0 1.88-.34l.06-.06 2.86 2.86-.06.06A1.7 1.7 0 0 0 19.4 8.5a1.7 1.7 0 0 0 .6 1 1.7 1.7 0 0 0 1.1.4h.1v4h-.1a1.7 1.7 0 0 0-1.7 1.1Z"/></svg>设备</button>
  </nav>
</div>

<div class="toast" id="toast" role="status"></div>
<dialog id="confirmDialog"><div class="dialog-box"><h3>安全故障复位</h3><p>请确认压力已释放、温度低于 105°C、旋钮在关断位。只在排除故障后继续。</p><div class="dialog-actions"><button class="secondary" id="cancelReset">取消</button><button class="primary" id="confirmReset">已确认，复位</button></div></div></dialog>

<script>
(() => {
  const $ = (id) => document.getElementById(id);
  const stateMeta = {
    BOOT:["启动中","系统正在初始化",""],
    IDLE:["预热中","等待锅炉达到目标温度",""],
    READY:["就绪","可以开始自动萃取","ready"],
    PRE_INFUSION:["预浸泡","低压建立中","active"],
    PRESSURE_RAMP:["建压中","压力正在接近目标","active"],
    EXTRACTION:["萃取中","压力闭环运行","active"],
    STOP:["回流中","泵输出正在归零","active"],
    MANUAL:["手动模式","旋钮拥有最高操作优先级","active"],
    FAULT:["安全故障","已锁存，需确认后复位","fault"]
  };
  let status = null;
  let settings = null;
  let points = [];
  let toastTimer = 0;
  let pollTimer = 0;

  function toast(text){
    const el = $("toast");
    el.textContent = text;
    el.classList.add("show");
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => el.classList.remove("show"), 2300);
  }

  function fixed(value, digits=1){
    if(value === null || value === undefined || value === "") return "--";
    return Number.isFinite(Number(value)) ? Number(value).toFixed(digits) : "--";
  }

  function setRangeFill(input){
    const min = Number(input.min), max = Number(input.max), value = Number(input.value);
    const ratio = max === min ? 0 : (value-min)/(max-min)*100;
    input.style.setProperty("--fill", `${Math.max(0,Math.min(100,ratio))}%`);
  }

  function bindInput(id, formatter, outputId){
    const input = $(id), output = $(outputId);
    const refresh = () => {
      setRangeFill(input);
      output.textContent = formatter(Number(input.value));
    };
    input.addEventListener("input", refresh);
    refresh();
    return refresh;
  }

  const refreshers = [
    bindInput("preVoltage", v => `${v.toFixed(2)} V`, "preVoltageOut"),
    bindInput("preTime", v => `${v.toFixed(1)} s`, "preTimeOut"),
    bindInput("extractionPressure", v => `${v.toFixed(1)} bar`, "extractionPressureOut"),
    bindInput("extractionTime", v => `${v.toFixed(0)} s`, "extractionTimeOut"),
    bindInput("maxPressure", v => `${v.toFixed(1)} bar`, "maxPressureOut"),
    bindInput("boilerSp", v => `${v.toFixed(1)} °C`, "boilerSpOut"),
    bindInput("pidKp", v => v.toFixed(2), "pidKpOut"),
    bindInput("pidKi", v => v.toFixed(2), "pidKiOut"),
    bindInput("pidKd", v => v.toFixed(2), "pidKdOut")
  ];

  function setConnection(online){
    const el = $("connection");
    el.classList.toggle("offline", !online);
    el.textContent = online ? "设备在线" : "连接中断";
  }

  function isBrewing(s){
    return ["PRE_INFUSION","PRESSURE_RAMP","EXTRACTION","STOP"].includes(s.brewState);
  }

  function renderStatus(s){
    status = s;
    const meta = stateMeta[s.brewState] || ["未知状态","--","fault"];
    $("stateName").textContent = meta[0];
    $("stateHint").textContent = meta[1];
    $("stateDot").className = `state-dot ${meta[2]}`;
    $("tempValue").textContent = fixed(s.tempPv,1);
    $("tempTarget").textContent = fixed(s.tempSp,1);
    $("pressureValue").textContent = fixed(s.pressure,2);
    $("pumpVoltageInline").textContent = fixed(s.pumpVoltage,2);
    $("pumpSource").textContent = ({AUTO:"自动",MANUAL:"旋钮",SAFETY:"安全锁止",IDLE:"待机"}[s.pumpSource] || "--");
    $("brewElapsed").textContent = `${((s.brewElapsedMs||0)/1000).toFixed(1)}s`;
    $("commsState").textContent = s.modbusOnline ? "在线" : "离线";
    $("tempMeter").style.width = `${Math.max(0,Math.min(100,((s.tempPv||0)/105)*100))}%`;
    $("pressureMeter").style.width = `${Math.max(0,Math.min(100,((s.pressure||0)/12)*100))}%`;
    $("faultBanner").classList.toggle("show", s.faultMask !== 0);
    $("faultBanner").textContent = s.faultMask ? `安全锁已触发：${s.faultText || "未知故障"}。泵输出已强制归零。` : "";
    $("modbusPill").textContent = s.modbusOnline ? "正常" : "异常";
    $("modbusPill").classList.toggle("bad", !s.modbusOnline);
    $("modbusDetail").textContent = s.modbusOnline ? "E5CC 回读正常" : "检查地址、波特率和校验位";
    $("sensorPill").textContent = s.pressureSensorValid ? "正常" : "异常";
    $("sensorPill").classList.toggle("bad", !s.pressureSensorValid);
    $("sensorDetail").textContent = s.pressureSensorValid ? "ADC 信号有效" : "检查分压与接线";
    $("ipDetail").textContent = s.ip || "--";
    $("wifiPill").textContent = Number.isFinite(s.rssi) && s.rssi < 0 ? `${s.rssi} dBm` : (s.networkMode || "AP");
    $("apDetail").textContent = s.accessPointActive ? `${s.accessPointSsid} · http://${s.accessPointIp}` : "未开启";
    $("apPill").textContent = s.accessPointActive ? "热点" : "关闭";
    $("boardDetail").textContent = s.board || "--";
    $("firmwareVersion").textContent = s.firmware ? `v${s.firmware}` : "--";
    $("heapDetail").textContent = `${Math.round((s.heapFree||0)/1024)} KB`;

    const brewing = isBrewing(s);
    const manual = s.brewState === "MANUAL";
    const fault = s.faultMask !== 0 || s.brewState === "FAULT";
    const ready = s.brewState === "READY";
    const button = $("brewButton");
    button.disabled = fault || manual || (!brewing && !ready);
    button.classList.toggle("stop", brewing);
    $("brewButtonText").textContent = brewing ? "停止" : "开始";
    button.setAttribute("aria-label", brewing ? "停止萃取" : "开始萃取");
    $("brewTitle").textContent = brewing ? `${meta[0]} · ${((s.brewElapsedMs||0)/1000).toFixed(1)}s` : (manual ? "手动控制中" : "自动萃取");
    $("brewDescription").textContent = brewing ? "点击停止会立即进入泵归零流程" : manual ? "旋钮离开关断位时自动程序保持中止" : (ready ? "锅炉已到位，可以开始一杯" : "预热完成后即可开始一杯");

    points.push({p:s.pressure, t:s.tempPv, time:Date.now()});
    if(points.length > 140) points.shift();
    drawChart();
  }

  function drawChart(){
    const canvas = $("liveChart");
    const rect = canvas.getBoundingClientRect();
    const dpr = Math.min(window.devicePixelRatio || 1, 2);
    const w = Math.max(1, rect.width), h = Math.max(1, rect.height);
    if(canvas.width !== Math.round(w*dpr) || canvas.height !== Math.round(h*dpr)){
      canvas.width = Math.round(w*dpr); canvas.height = Math.round(h*dpr);
    }
    const ctx = canvas.getContext("2d");
    ctx.setTransform(dpr,0,0,dpr,0,0);
    ctx.clearRect(0,0,w,h);
    ctx.strokeStyle = "rgba(255,235,213,.07)"; ctx.lineWidth = 1;
    for(let i=0;i<4;i++){const y=8+i*(h-16)/3;ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(w,y);ctx.stroke();}
    if(points.length < 2) return;
    const drawLine = (key, min, max, color, fill) => {
      const valid = points.filter(p => Number.isFinite(Number(p[key])));
      if(valid.length < 2) return;
      const xAt = i => i/(points.length-1)*w;
      const yAt = p => h-5-Math.max(0,Math.min(1,((Number(p[key])||0)-min)/(max-min)))*(h-10);
      if(fill){
        const grad=ctx.createLinearGradient(0,0,0,h);
        grad.addColorStop(0,fill);grad.addColorStop(1,"rgba(232,183,111,0)");
        ctx.beginPath();ctx.moveTo(0,h);
        points.forEach((p,i)=>ctx.lineTo(xAt(i),yAt(p)));
        ctx.lineTo(w,h);ctx.closePath();ctx.fillStyle=grad;ctx.fill();
      }
      ctx.beginPath();
      points.forEach((p,i)=>{const x=xAt(i),y=yAt(p);i?ctx.lineTo(x,y):ctx.moveTo(x,y)});
      ctx.strokeStyle=color;ctx.lineWidth=2.2;ctx.lineJoin="round";ctx.lineCap="round";ctx.stroke();
    };
    drawLine("p",0,12,"#e8b76f","rgba(232,183,111,.13)");
    drawLine("t",60,105,"rgba(120,183,217,.72)",null);
  }

  async function getJson(url, options={}){
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), 3500);
    try{
      const response = await fetch(url, {...options, signal:controller.signal, cache:"no-store"});
      const data = await response.json().catch(()=>({}));
      if(!response.ok) throw new Error(data.error || `HTTP ${response.status}`);
      return data;
    } finally { clearTimeout(timeout); }
  }

  async function poll(){
    clearTimeout(pollTimer);
    try{
      renderStatus(await getJson("/api/status"));
      setConnection(true);
    }catch(error){
      setConnection(false);
    }
    pollTimer=setTimeout(poll, document.hidden ? 2500 : 750);
  }

  function populateSettings(s){
    settings = s;
    const values = {
      preVoltage:[s.preVoltage,2], preTime:[s.preTimeMs/1000,1],
      extractionPressure:[s.extractionPressure,1], extractionTime:[s.extractionTimeMs/1000,0],
      maxPressure:[s.maxPressure,1], boilerSp:[s.boilerSp,1],
      pidKp:[s.pidKp,2], pidKi:[s.pidKi,2], pidKd:[s.pidKd,2]
    };
    Object.entries(values).forEach(([id,[value,digits]]) => {
      const input=$(id); input.value=Number(value).toFixed(digits); setRangeFill(input);
    });
    refreshers.forEach(fn=>fn());
  }

  async function loadSettings(){
    try{ populateSettings(await getJson("/api/settings")); }
    catch(error){ toast("配方读取失败"); }
  }

  function collectSettings(includeRecipe=true, includeSystem=true){
    const body={};
    if(includeRecipe){
      body.preVoltage=Number($("preVoltage").value);
      body.preTimeMs=Math.round(Number($("preTime").value)*1000);
      body.extractionPressure=Number($("extractionPressure").value);
      body.extractionTimeMs=Math.round(Number($("extractionTime").value)*1000);
      body.maxPressure=Number($("maxPressure").value);
    }
    if(includeSystem){
      body.boilerSp=Number($("boilerSp").value);
      body.pidKp=Number($("pidKp").value);
      body.pidKi=Number($("pidKi").value);
      body.pidKd=Number($("pidKd").value);
    }
    return body;
  }

  async function saveSettings(includeRecipe, includeSystem){
    const button = includeRecipe ? $("saveRecipe") : $("saveSystem");
    button.disabled=true;
    try{
      await getJson("/api/settings",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(collectSettings(includeRecipe,includeSystem))});
      toast("设置已保存");
      await loadSettings();
      await poll();
    }catch(error){
      toast(error.message === "stop_before_saving" ? "请先停止当前萃取或手动输出" : "保存失败，请检查连接");
    }finally{button.disabled=false;}
  }

  $("saveRecipe").addEventListener("click",()=>saveSettings(true,false));
  $("saveSystem").addEventListener("click",()=>saveSettings(false,true));

  $("brewButton").addEventListener("click", async()=>{
    if(!status) return;
    const action=isBrewing(status)?"stop":"start";
    $("brewButton").disabled=true;
    try{
      await getJson("/api/brew",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({action})});
      setTimeout(poll,180);
    }catch(error){
      toast(action==="start" ? "未达到启动条件或指令队列忙" : "停止指令发送失败");
      setTimeout(poll,450);
    }
  });

  $("resetFault").addEventListener("click",()=>{
    if(!status || !status.faultMask){toast("当前没有锁存故障");return;}
    $("confirmDialog").showModal();
  });
  $("cancelReset").addEventListener("click",()=>$("confirmDialog").close());
  $("confirmReset").addEventListener("click", async()=>{
    $("confirmDialog").close();
    try{
      await getJson("/api/fault/reset",{method:"POST",headers:{"Content-Type":"application/json"},body:"{}"});
      toast("已提交复位检查");
      setTimeout(poll,300);
    }catch(error){toast("复位指令发送失败");}
  });

  document.querySelectorAll(".nav button").forEach(button=>{
    button.addEventListener("click",()=>{
      document.querySelectorAll(".nav button").forEach(item=>item.classList.toggle("active",item===button));
      document.querySelectorAll(".view").forEach(view=>view.hidden=view.dataset.view!==button.dataset.tab);
      window.scrollTo({top:0,behavior:"smooth"});
      if(button.dataset.tab==="overview") requestAnimationFrame(drawChart);
    });
  });

  window.addEventListener("resize",drawChart);
  document.addEventListener("visibilitychange",()=>{if(!document.hidden)poll();});
  loadSettings();
  poll();
})();
</script>
</body>
</html>)COFFEEHTML";

#endif  // COFFEE_CONTROLLER_WEB_APP_H_
