#include "web_page.h"

// Single page web interface. Data is fetched from the /data/*.json endpoints
// and rendered with textContent only (never innerHTML), so device names or MQTT
// payloads can not inject markup.
const char WEB_INDEX_HTML[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>WS2MQTT Gateway</title>
<link rel="icon" type="image/svg+xml" href="favicon.svg">
<style>
:root {
  --bg: #f4f5f7; --card: #ffffff; --text: #1d2330; --muted: #667085; --line: #e3e6ec;
  --ok: #1a7f4b; --ok-bg: #e2f5ea; --bad: #b42318; --bad-bg: #fde8e6;
  --warn: #9a5b00; --warn-bg: #fff1d6; --idle: #475467; --idle-bg: #eceff4; --accent: #2f5fd0;
}
@media (prefers-color-scheme: dark) {
  :root {
    --bg: #12151c; --card: #1b2029; --text: #e6e9f0; --muted: #98a2b3; --line: #2b3240;
    --ok: #5fd397; --ok-bg: #173826; --bad: #ff8b80; --bad-bg: #44201d;
    --warn: #f5c25e; --warn-bg: #3d2f12; --idle: #b6bfce; --idle-bg: #272e3a; --accent: #7ba2ff;
  }
}
* { box-sizing: border-box; }
body { margin: 0; background: var(--bg); color: var(--text);
  font: 15px/1.45 system-ui, -apple-system, "Segoe UI", Roboto, sans-serif; }
header { padding: 14px 16px; display: flex; flex-wrap: wrap; gap: 10px 16px; align-items: center;
  background: var(--card); border-bottom: 1px solid var(--line); position: sticky; top: 0; z-index: 2; }
h1 { font-size: 18px; margin: 0; }
.ver { color: var(--muted); font-size: 13px; }
.pills { display: flex; flex-wrap: wrap; gap: 6px; margin-left: auto; }
main { max-width: 1100px; margin: 0 auto; padding: 16px; display: grid; gap: 16px; }
.banner { background: var(--bad-bg); color: var(--bad); border: 1px solid var(--bad);
  padding: 12px 14px; border-radius: 8px; font-weight: 600; }
.card { background: var(--card); border: 1px solid var(--line); border-radius: 10px; padding: 14px 16px; }
.card h2 { font-size: 15px; margin: 0 0 10px; }
.card h3 { font-size: 14px; margin: 16px 0 6px; }
.grid2 { display: grid; gap: 16px; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
.scroll { overflow-x: auto; }
table { border-collapse: collapse; width: 100%; font-size: 14px; }
th, td { text-align: left; padding: 6px 10px 6px 0; border-bottom: 1px solid var(--line); vertical-align: top; }
th { color: var(--muted); font-weight: 600; font-size: 12px; text-transform: uppercase; letter-spacing: .03em; white-space: nowrap; }
tr:last-child td { border-bottom: 0; }
td.mono, .mono { font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace; font-size: 13px; }
td.wrap { word-break: break-all; }
.kv { display: grid; grid-template-columns: minmax(110px, 38%) 1fr; gap: 5px 12px; margin: 0; }
.kv dt { color: var(--muted); }
.kv dd { margin: 0; word-break: break-word; }
.pill { display: inline-block; padding: 1px 9px; border-radius: 999px; font-size: 12px; font-weight: 600;
  background: var(--idle-bg); color: var(--idle); white-space: nowrap; }
.pill.ok { background: var(--ok-bg); color: var(--ok); }
.pill.bad { background: var(--bad-bg); color: var(--bad); }
.pill.warn { background: var(--warn-bg); color: var(--warn); }
.sub { color: var(--muted); font-size: 12px; }
.bar { display: flex; flex-wrap: wrap; gap: 14px; align-items: center; margin-bottom: 8px; color: var(--muted); font-size: 13px; }
.bar label { display: inline-flex; gap: 6px; align-items: center; cursor: pointer; }
input.loc { font: inherit; width: 9em; padding: 2px 6px; border-radius: 6px; border: 1px solid var(--line);
  background: var(--card); color: var(--text); }
button { font: inherit; padding: 3px 12px; border-radius: 6px; border: 1px solid var(--line);
  background: var(--card); color: var(--text); cursor: pointer;
  transition: background-color .15s, color .15s, border-color .15s; }
button.sent { background: var(--ok-bg); color: var(--ok); border-color: var(--ok); }
.empty { color: var(--muted); padding: 6px 0; }
.dir-in { color: var(--accent); font-weight: 600; }
footer { text-align: center; color: var(--muted); font-size: 12px; padding: 0 0 24px; }
</style>
</head>
<body>
<header>
  <h1>WS2MQTT Gateway</h1>
  <span class="ver" id="hdrVer"></span>
  <div class="pills" id="pills"></div>
</header>
<main>
  <div class="banner" id="alarmBanner" hidden></div>

  <section class="card">
    <h2>WiSafe2 devices</h2>
    <div class="bar">
      <button id="probeAllBtn" type="button">Probe all devices</button>
      <button id="markDockedBtn" type="button">Mark all docked</button>
      <button id="markBatteryBtn" type="button">Mark all battery OK</button>
      <button id="forgetAllBtn" type="button">Forget all devices</button>
      <label><input type="checkbox" id="showUnid"> Show unidentified SIDs <span id="unidCount"></span></label>
      <span>Probe asks devices for their status. Forget removes devices from this gateway and Home Assistant; devices still on the radio network are found again within a minute.</span>
    </div>
    <div class="scroll" id="devices"><div class="empty">Loading...</div></div>
    <div id="unid" hidden></div>
  </section>

  <div class="grid2">
    <section class="card"><h2>WiSafe2 radio module</h2><dl class="kv" id="module"></dl></section>
    <section class="card"><h2>ESP32</h2><dl class="kv" id="system"></dl></section>
    <section class="card"><h2>Network &amp; MQTT</h2><dl class="kv" id="network"></dl></section>
  </div>

  <section class="card">
    <h2>Recent MQTT messages</h2>
    <div class="bar">
      <label><input type="checkbox" id="showHk"> Include discovery &amp; bridge sensor messages</label>
      <button id="pauseBtn" type="button">Pause</button>
      <span id="mqttCount"></span>
    </div>
    <div class="scroll" id="mqtt"><div class="empty">Loading...</div></div>
  </section>
</main>
<footer id="updated"></footer>

<script>
"use strict";
var paused = false;
var editing = false;

function el(tag, cls, text) {
  var e = document.createElement(tag);
  if (cls) e.className = cls;
  if (text !== undefined && text !== null) e.textContent = text;
  return e;
}
function pill(text, kind) { return el("span", "pill" + (kind ? " " + kind : ""), text); }

function fmtAge(ms) {
  if (ms === null || ms === undefined) return "never";
  var s = Math.floor(ms / 1000);
  if (s < 2) return "just now";
  if (s < 60) return s + "s ago";
  var m = Math.floor(s / 60);
  if (m < 60) return m + "m " + (s % 60) + "s ago";
  var h = Math.floor(m / 60);
  if (h < 48) return h + "h " + (m % 60) + "m ago";
  return Math.floor(h / 24) + "d " + (h % 24) + "h ago";
}
function fmtDuration(s) {
  var d = Math.floor(s / 86400), h = Math.floor(s % 86400 / 3600),
      m = Math.floor(s % 3600 / 60), r = s % 60, out = [];
  if (d) out.push(d + "d");
  if (d || h) out.push(h + "h");
  if (d || h || m) out.push(m + "m");
  out.push(r + "s");
  return out.join(" ");
}
function fmtBytes(b) { return b >= 1048576 ? (b / 1048576).toFixed(1) + " MB" : (b / 1024).toFixed(1) + " KB"; }

function fillKv(id, rows) {
  var dl = document.getElementById(id);
  dl.replaceChildren();
  rows.forEach(function (r) {
    dl.appendChild(el("dt", null, r[0]));
    var dd = el("dd");
    if (r[1] instanceof Node) dd.appendChild(r[1]); else dd.textContent = r[1];
    dl.appendChild(dd);
  });
}
function yesNo(v, goodWhenTrue) {
  var good = goodWhenTrue ? v : !v;
  return pill(v ? "Yes" : "No", good ? "ok" : "bad");
}

function get(url) {
  return fetch(url, { cache: "no-store" }).then(function (r) {
    if (!r.ok) throw new Error(url + " " + r.status);
    return r.json();
  });
}

// Write requests need this header, which other websites cannot add
function post(url) {
  return fetch(url, { method: "POST", headers: { "X-Requested-With": "ws2mqtt" } }).then(function (r) {
    if (!r.ok) throw new Error(url + " " + r.status);
    return r.json();
  });
}

// A button turns green and says "Sent" for a moment after it is clicked. The
// table is rebuilt often, so the state is kept here and put back on new buttons.
var FLASH_MS = 1500;
var flashes = {};
var flashBtns = {};
function paintFlash(key) {
  var b = flashBtns[key];
  if (!b) return;
  var on = (flashes[key] || 0) > Date.now();
  b.className = on ? "sent" : "";
  b.textContent = on ? "Sent \u2713" : b.idleText;
}
function actionButton(key, label) {
  var b = el("button", null, label);
  b.type = "button";
  b.idleText = label;
  flashBtns[key] = b;
  paintFlash(key);
  return b;
}
function flash(key) {
  flashes[key] = Date.now() + FLASH_MS;
  paintFlash(key);
  setTimeout(function () { paintFlash(key); }, FLASH_MS + 50);
}
function unflash(key) {
  delete flashes[key];
  paintFlash(key);
}

function renderDevices(data) {
  var box = document.getElementById("devices");
  box.replaceChildren();
  if (!data.devices.length) {
    box.appendChild(el("div", "empty", "No devices paired yet."));
    return 0;
  }
  var table = el("table"), head = el("tr");
  ["SID", "Device ID", "Location", "Model", "Type", "Alarm", "Docked", "Battery", "Faults", "Signal", "Last heard", ""].forEach(function (h) {
    head.appendChild(el("th", null, h));
  });
  table.appendChild(head);

  var alarms = 0;
  data.devices.forEach(function (d) {
    var tr = el("tr");
    tr.appendChild(el("td", "mono", d.sid));
    tr.appendChild(el("td", "mono", d.device_id));

    var loc = el("td");
    var inp = el("input", "loc");
    inp.type = "text";
    inp.maxLength = 32;
    inp.placeholder = "e.g. Kitchen";
    inp.value = d.location || "";
    inp.addEventListener("focus", function () { editing = true; });
    inp.addEventListener("blur", function () { editing = false; });
    inp.addEventListener("keydown", function (e) { if (e.key === "Enter") inp.blur(); });
    inp.addEventListener("change", function () {
      post("api/location?sid=" + d.sid + "&value=" + encodeURIComponent(inp.value))
        .then(refresh).catch(function () { alert("Could not save the location"); });
    });
    loc.appendChild(inp);
    tr.appendChild(loc);

    var model = el("td");
    model.appendChild(document.createTextNode(d.model));
    model.appendChild(el("div", "sub mono", d.model_id));
    tr.appendChild(model);

    var type = el("td");
    type.appendChild(document.createTextNode(d.type));
    type.appendChild(el("div", "sub mono", d.type_id));
    tr.appendChild(type);

    var alarm = el("td");
    if (d.alarm === true) { alarms++; alarm.appendChild(pill("ALARM", "bad")); }
    else if (d.alarm === false) alarm.appendChild(pill("OK", "ok"));
    else alarm.appendChild(pill("None reported"));
    tr.appendChild(alarm);

    var isControlUnit = (d.type_id === "0xC4");
    var docked = el("td");
    if (isControlUnit) docked.appendChild(pill("N/A"));
    else docked.appendChild(d.state && d.state.docked !== null ? pill(d.state.docked ? "Docked" : "Not docked", d.state.docked ? "ok" : "warn") : pill("None reported"));
    tr.appendChild(docked);

    var battery = el("td");
    if (isControlUnit) battery.appendChild(pill("N/A"));
    else if (!d.state || d.state.sensor_battery === null) battery.appendChild(pill("None reported"));
    else {
      if (d.state.sensor_battery) battery.appendChild(pill("Sensor battery low", "warn"));
      if (d.state.radio_module_battery) battery.appendChild(pill("Radio battery low", "warn"));
      if (!d.state.sensor_battery && !d.state.radio_module_battery) battery.appendChild(pill("OK", "ok"));
    }
    tr.appendChild(battery);

    var faults = el("td");
    if (isControlUnit) {
      // No battery or dock state; only an error is worth showing
      faults.appendChild(d.state && d.state.generic_error ? pill("Error", "bad") : pill("N/A"));
    } else if (!d.state) faults.appendChild(pill("None reported"));
    else {
      faults.appendChild(d.state.generic_error ? pill("Error", "bad") : pill("None", "ok"));
    }
    tr.appendChild(faults);

    var sig = el("td");
    if (d.diag) {
      sig.appendChild(document.createTextNode("RSSI " + d.diag.rssi));
      sig.appendChild(el("div", "sub", "faults " + d.diag.radio_fault_count));
    } else sig.textContent = "-";
    tr.appendChild(sig);

    var seen = el("td");
    seen.appendChild(document.createTextNode(fmtAge(d.last_seen_age_ms)));
    if (d.last_alarm_age_ms !== null) seen.appendChild(el("div", "sub", "alarm " + fmtAge(d.last_alarm_age_ms)));
    if (d.last_button_age_ms !== null) seen.appendChild(el("div", "sub", "test " + fmtAge(d.last_button_age_ms)));
    var sq = d.status_queries;
    if (sq && sq.sent > 0) {
      var txt = "status replies " + sq.replies + "/" + sq.sent;
      if (sq.no_answer > 0) txt += ", no answer " + sq.no_answer;
      if (sq.last_reply_age_ms !== null) txt += ", last reply " + fmtAge(sq.last_reply_age_ms);
      seen.appendChild(el("div", "sub", txt));
      if (sq.raw) seen.appendChild(el("div", "sub mono", "raw " + sq.raw));
    }
    tr.appendChild(seen);

    var act = el("td");
    var pkey = "probe" + d.sid;
    var pb = actionButton(pkey, "Probe");
    pb.addEventListener("click", function () {
      flash(pkey);
      post("api/probe?sid=" + d.sid).then(refresh).catch(function () { unflash(pkey); alert("Could not probe the device"); });
    });
    act.appendChild(pb);
    act.appendChild(document.createTextNode(" "));
    var fb = el("button", null, "Forget");
    fb.type = "button";
    fb.addEventListener("click", function () {
      if (!confirm("Forget device " + d.device_id + " (SID " + d.sid + ")?\n\nIt is removed from this gateway and Home Assistant, and its location is cleared. If it is still on the radio network it will be found again within a minute.")) return;
      post("api/forget?sid=" + d.sid).then(refresh).catch(function () { alert("Could not forget the device"); });
    });
    act.appendChild(fb);
    tr.appendChild(act);

    table.appendChild(tr);
  });
  box.appendChild(table);
  return alarms;
}

// SIDs that are on the radio network but have not told us what they are. Shown
// only when the "Show unidentified SIDs" box is ticked.
function renderUnidentified(list) {
  var box = document.getElementById("unid");
  var shown = document.getElementById("showUnid").checked;
  document.getElementById("unidCount").textContent = "(" + list.length + ")";
  box.hidden = !shown;
  box.replaceChildren();
  if (!shown) return;
  box.appendChild(el("h3", null, "Unidentified SIDs"));
  if (!list.length) {
    box.appendChild(el("div", "empty", "None. Every SID on the radio network is a known device."));
    return;
  }
  box.appendChild(el("div", "empty", "These are on the radio network but have not answered the identity request yet. The gateway asks again about once a minute."));
  var table = el("table"), head = el("tr");
  ["SID", "Identity requests", "No answer", "Last asked"].forEach(function (h) { head.appendChild(el("th", null, h)); });
  table.appendChild(head);
  list.forEach(function (u) {
    var tr = el("tr");
    tr.appendChild(el("td", "mono", u.sid));
    tr.appendChild(el("td", null, u.id_queries));
    tr.appendChild(el("td", null, u.id_no_answer));
    tr.appendChild(el("td", null, u.id_queries ? fmtAge(u.last_query_age_ms) : "not yet"));
    table.appendChild(tr);
  });
  box.appendChild(table);
}

function renderModule(m) {
  var rows = [];
  rows.push(["Paired", m.paired === null ? pill("Waiting for module") : yesNo(m.paired, true)]);
  rows.push(["Module SID", m.own_sid === null ? "-" : String(m.own_sid)]);
  rows.push(["Gateway ID", m.gateway_device_id]);
  if (m.diag) {
    rows.push(["Module device ID", m.diag.device_id + (m.diag.initialised ? "" : " (not initialised)")]);
    rows.push(["Module firmware (raw)", String(m.diag.firmware_version)]);
    rows.push(["Battery (raw)", m.diag.battery1 + " / " + m.diag.battery2]);
    rows.push(["RSSI (raw)", String(m.diag.rssi)]);
    rows.push(["Flags", m.diag.flags]);
    rows.push(["Radio faults", String(m.diag.radio_fault_count)]);
    rows.push(["Last diagnostic", fmtAge(m.diag.age_ms)]);
  } else rows.push(["Diagnostic", "No response yet"]);
  if (m.sidmap) {
    rows.push(["SIDs on network", m.sidmap.sids.length ? m.sidmap.sids.join(", ") : "none"]);
    rows.push(["SID map", m.sidmap.hex + " (" + fmtAge(m.sidmap.age_ms) + ")"]);
  } else rows.push(["SID map", "Not received yet"]);
  rows.push(["Welcome handshakes", m.welcome.count + " (" + fmtAge(m.welcome.age_ms) + ")"]);
  rows.push(["Module responding", yesNo(m.rx.responding, true)]);
  rows.push(["Frames received", m.rx.frames + " (" + fmtAge(m.rx.age_ms) + ")"]);
  rows.push(["RX problems", m.rx.frame_timeouts + " timeouts, " + m.rx.bad_length + " bad length, " + m.rx.short_packets + " short"]);
  if (m.unrecognised) {
    rows.push(["Unrecognised packets", String(m.unrecognised.total)]);
    m.unrecognised.types.forEach(function (u) {
      var label = "Type " + u.type + (u.sub ? " / " + u.sub : "");
      rows.push([label, u.count + " times, last " + fmtAge(u.age_ms) + ": " + u.bytes]);
    });
  }
  rows.push(["Messages sent", m.tx.sent + " (" + fmtAge(m.tx.age_ms) + ")"]);
  rows.push(["TX queue", m.tx.queued + " waiting, " + m.tx.dropped + " dropped"]);
  fillKv("module", rows);
}

function renderSystem(s) {
  fillKv("system", [
    ["Firmware", s.firmware],
    ["Built", s.build],
    ["Uptime", fmtDuration(s.uptime_s)],
    ["Boot reason", s.reset_reason],
    ["Chip", s.chip.model + " rev " + s.chip.revision + ", " + s.chip.cores + " cores @ " + s.chip.cpu_mhz + " MHz"],
    ["Flash", fmtBytes(s.chip.flash_bytes) + " (firmware " + fmtBytes(s.chip.sketch_bytes) + ")"],
    ["Heap free", fmtBytes(s.heap.free) + " of " + fmtBytes(s.heap.total) + " (lowest " + fmtBytes(s.heap.min_free) + ")"],
    ["Bridge name", s.bridge.name],
    ["Bridge ID", s.bridge.device_id],
    ["SDK", s.chip.sdk]
  ]);
  fillKv("network", [
    ["WiFi", s.wifi.connected ? pill("Connected", "ok") : pill("Disconnected", "bad")],
    ["Network (SSID)", s.wifi.ssid],
    ["Signal", s.wifi.rssi + " dBm, channel " + s.wifi.channel],
    ["IP address", s.wifi.ip],
    ["MAC address", s.wifi.mac],
    ["BSSID", s.wifi.bssid],
    ["Hostname", s.wifi.hostname],
    ["MQTT broker", s.mqtt.host],
    ["MQTT", s.mqtt.connected ? pill(s.mqtt.state_text, "ok") : pill(s.mqtt.state_text, "bad")],
    ["MQTT connects", s.mqtt.connects + " of " + s.mqtt.connect_attempts + " attempts" +
      (s.mqtt.last_fail_text ? " (last failure: " + s.mqtt.last_fail_text + ")" : "")],
    ["Last connected", fmtAge(s.mqtt.last_connect_age_ms)],
    ["MQTT sent", s.mqtt.published + " (" + s.mqtt.publish_failed + " failed)"],
    ["MQTT received", String(s.mqtt.received)]
  ]);
}

function renderPills(s, m, alarms) {
  var p = document.getElementById("pills");
  p.replaceChildren();
  p.appendChild(pill("WiFi " + s.wifi.rssi + " dBm", s.wifi.connected ? "ok" : "bad"));
  p.appendChild(pill("MQTT " + (s.mqtt.connected ? "connected" : "down"), s.mqtt.connected ? "ok" : "bad"));
  p.appendChild(pill(m.paired === null ? "Module: waiting" : (m.paired ? "Module paired" : "Module not paired"),
    m.paired === null ? "" : (m.paired ? "ok" : "bad")));
  p.appendChild(pill(alarms ? alarms + " in alarm" : "No alarms", alarms ? "bad" : "ok"));
  document.getElementById("hdrVer").textContent = "v" + s.firmware;
}

function renderMqtt(data) {
  var box = document.getElementById("mqtt");
  box.replaceChildren();
  document.getElementById("mqttCount").textContent = data.messages.length + " shown";
  if (!data.messages.length) {
    box.appendChild(el("div", "empty", "No messages yet."));
    return;
  }
  var table = el("table"), head = el("tr");
  ["Time", "Dir", "Topic", "Payload", ""].forEach(function (h) { head.appendChild(el("th", null, h)); });
  table.appendChild(head);
  data.messages.forEach(function (m) {
    var tr = el("tr");
    tr.appendChild(el("td", null, fmtAge(m.age_ms)));
    tr.appendChild(el("td", m.dir === "in" ? "dir-in" : null, m.dir === "in" ? "IN" : "OUT"));
    tr.appendChild(el("td", "mono wrap", m.topic));
    var pl = m.payload + (m.payload_len > m.payload.length ? " ... (" + m.payload_len + " bytes)" : "");
    tr.appendChild(el("td", "mono wrap", pl));
    var flags = el("td");
    if (m.retained) flags.appendChild(pill("retained"));
    if (!m.ok) flags.appendChild(pill("failed", "bad"));
    tr.appendChild(flags);
    table.appendChild(tr);
  });
  box.appendChild(table);
}

function setBanner(alarms) {
  var b = document.getElementById("alarmBanner");
  b.hidden = !alarms;
  b.textContent = alarms ? (alarms === 1 ? "1 detector is in alarm" : alarms + " detectors are in alarm") : "";
}

var failures = 0;
function refresh() {
  if (paused) return Promise.resolve();
  var all = document.getElementById("showHk").checked ? "1" : "0";
  var sys, mod, dev;
  return get("data/system.json").then(function (r) { sys = r; return get("data/module.json"); })
    .then(function (r) { mod = r; return get("data/devices.json"); })
    .then(function (r) { dev = r; return get("data/mqtt.json?all=" + all); })
    .then(function (mq) {
      // Leave the table alone while a location is being typed
      var alarms = editing
        ? dev.devices.filter(function (x) { return x.alarm === true; }).length
        : renderDevices(dev);
      renderUnidentified(dev.unidentified || []);
      renderModule(mod);
      renderSystem(sys);
      renderPills(sys, mod, alarms);
      renderMqtt(mq);
      setBanner(alarms);
      failures = 0;
      document.getElementById("updated").textContent = "Updated " + new Date().toLocaleTimeString() + " - refreshes every 3 seconds";
    })
    .catch(function () {
      failures++;
      document.getElementById("updated").textContent = "Cannot reach the gateway (retrying)...";
    });
}

flashBtns.probeAll = document.getElementById("probeAllBtn");
flashBtns.probeAll.idleText = flashBtns.probeAll.textContent;
flashBtns.probeAll.addEventListener("click", function () {
  flash("probeAll");
  post("api/probe-all").then(refresh).catch(function () { unflash("probeAll"); alert("Could not probe the devices"); });
});

document.getElementById("markDockedBtn").addEventListener("click", function () {
  if (!confirm("Mark all devices as docked?\n\nThis records that every alarm (not control units) is docked, as the gateway cannot read that from a device. Devices that have reported they are not docked are left as they are. A real report from a device replaces it.")) return;
  post("api/mark-docked").then(function (r) {
    if (r.skipped_not_docked) alert(r.skipped_not_docked + " device(s) that reported they are not docked were left unchanged.");
    return refresh();
  }).catch(function () { alert("Could not mark the devices as docked"); });
});

document.getElementById("markBatteryBtn").addEventListener("click", function () {
  if (!confirm("Mark all batteries as OK?\n\nThis records that every alarm (not control units) has good batteries, as the gateway cannot read that until a device reports. Any device showing a fault is left as it is. A real report from a device replaces it.")) return;
  post("api/mark-battery-ok").then(function (r) {
    if (r.skipped_fault) alert(r.skipped_fault + " device(s) showing a fault were left unchanged.");
    return refresh();
  }).catch(function () { alert("Could not mark the batteries as OK"); });
});

document.getElementById("forgetAllBtn").addEventListener("click", function () {
  if (!confirm("Forget ALL devices?\n\nThey are removed from this gateway and Home Assistant. Devices still on the radio network are found again within a minute, with the locations you set.")) return;
  post("api/forget-all").then(refresh).catch(function () { alert("Could not forget the devices"); });
});

document.getElementById("pauseBtn").addEventListener("click", function () {
  paused = !paused;
  this.textContent = paused ? "Resume" : "Pause";
  if (!paused) refresh();
});
document.getElementById("showHk").addEventListener("change", function () { refresh(); });
document.getElementById("showUnid").addEventListener("change", function () { refresh(); });

function tick() { refresh().then(function () { setTimeout(tick, 3000); }); }
tick();
</script>
</body>
</html>
)HTML";

const char WEB_FAVICON_SVG[] PROGMEM =
	"<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 32 32\">"
	"<rect width=\"32\" height=\"32\" rx=\"7\" fill=\"#0f766e\"/>"
	"<circle cx=\"16\" cy=\"16\" r=\"3.5\" fill=\"#fff\"/>"
	"<path d=\"M10.5 10.5a7.8 7.8 0 0 0 0 11M21.5 10.5a7.8 7.8 0 0 1 0 11M6.5 6.5a13.4 13.4 0 0 0 0 19M25.5 6.5a13.4 13.4 0 0 1 0 19\" fill=\"none\" stroke=\"#fff\" stroke-width=\"2.6\" stroke-linecap=\"round\"/>"
	"</svg>";
