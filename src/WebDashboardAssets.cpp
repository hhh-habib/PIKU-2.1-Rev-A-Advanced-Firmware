#include "WebDashboardAssets.h"

const char DASHBOARD_HTML[] PROGMEM = R"html(
<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta name="theme-color" content="#07111f">
    <title>PIKU 2.1 Rev A Dashboard</title>
    <link rel="stylesheet" href="/dashboard.css">
  </head>
  <body>
    <div class="app-shell">
      <header class="app-header">
        <div class="header-main">
          <div class="brand">
            <span class="brand-mark" aria-hidden="true">P</span>
            <div>
              <h1>PIKU 2.1 Rev A</h1>
              <p>Embedded ESP32-S3 Robot</p>
            </div>
          </div>
          <div class="header-status">
            <span class="status-badge mode-badge">Mode: <strong data-bind="mode">--</strong></span>
            <span class="status-badge connection-badge is-updating" id="connectionState">
              <span class="connection-dot" aria-hidden="true"></span>
              <strong>Updating</strong>
            </span>
          </div>
        </div>
        <div class="alarm-indicator normal" id="alarmIndicator" role="status" aria-live="polite">
          <span class="alarm-label">SYSTEM NORMAL</span>
          <span class="alarm-detail">Waiting for telemetry</span>
        </div>
      </header>

      <nav class="section-nav" aria-label="Dashboard sections" role="tablist">
        <button class="nav-btn active" type="button" role="tab" aria-selected="true" aria-controls="overview" data-section="overview">Overview</button>
        <button class="nav-btn" type="button" role="tab" aria-selected="false" aria-controls="control" data-section="control">Control</button>
        <button class="nav-btn" type="button" role="tab" aria-selected="false" aria-controls="sensors" data-section="sensors">Sensors</button>
        <button class="nav-btn" type="button" role="tab" aria-selected="false" aria-controls="communication" data-section="communication">Communication</button>
        <button class="nav-btn" type="button" role="tab" aria-selected="false" aria-controls="system" data-section="system">System</button>
      </nav>

      <main class="dashboard-main">
        <section class="dashboard-section active" id="overview" role="tabpanel" aria-label="Overview">
          <div class="section-heading">
            <div>
              <p class="eyebrow">Mission overview</p>
              <h2>Robot at a glance</h2>
            </div>
            <button class="secondary-btn compact-btn" type="button" onclick="loadData()">Refresh telemetry</button>
          </div>

          <div class="summary-grid summary-grid-primary">
            <article class="metric-card safety-card">
              <span class="metric-label">Overall safety</span>
              <strong class="metric-value" data-bind="safetyState">--</strong>
              <span class="metric-note" data-bind="alarmReason">No active alarm</span>
            </article>
            <article class="metric-card">
              <span class="metric-label">Operating mode</span>
              <strong class="metric-value" data-bind="mode">--</strong>
              <span class="metric-note">Turn mode: <span data-bind="turnMode">--</span></span>
            </article>
            <article class="metric-card">
              <span class="metric-label">Current movement</span>
              <strong class="metric-value" data-bind="motorState">--</strong>
              <span class="metric-note" data-bind="navigationDecision">--</span>
            </article>
            <article class="metric-card">
              <span class="metric-label">Obstacle distance</span>
              <strong class="metric-value" data-bind="distance">-- cm</strong>
              <span class="metric-note">Scan: <span data-bind="scanOrientation">--</span></span>
            </article>
            <article class="metric-card">
              <span class="metric-label">Environmental hazard</span>
              <strong class="metric-value" data-bind="environmentHazard">--</strong>
              <span class="metric-note">MQ-2 relative / flame digital DO</span>
            </article>
            <article class="metric-card">
              <span class="metric-label">nRF24 link</span>
              <strong class="metric-value" data-bind="radioSummary">--</strong>
              <span class="metric-note">Standalone control remains local</span>
            </article>
            <article class="metric-card">
              <span class="metric-label">Sensor health</span>
              <strong class="metric-value" data-bind="sensorHealth">--</strong>
              <span class="metric-note" data-bind="sensorHealthDetail">--</span>
            </article>
          </div>

          <div class="overview-layout">
            <article class="panel">
              <div class="panel-heading">
                <div>
                  <p class="eyebrow">Stage-1 sensors</p>
                  <h3>Obstacle sensing summary</h3>
                </div>
                <span class="panel-badge" data-bind="safetyState">--</span>
              </div>
              <div class="mini-metric-grid">
                <div class="mini-metric">
                  <span>Left IR</span>
                  <strong data-bind="leftIrState">--</strong>
                </div>
                <div class="mini-metric">
                  <span>Right IR</span>
                  <strong data-bind="rightIrState">--</strong>
                </div>
                <div class="mini-metric">
                  <span>Left IR raw</span>
                  <strong data-bind="leftIrRaw">--</strong>
                </div>
                <div class="mini-metric">
                  <span>Right IR raw</span>
                  <strong data-bind="rightIrRaw">--</strong>
                </div>
                <div class="mini-metric">
                  <span>Near obstacle</span>
                  <strong data-bind="nearObstacleState">--</strong>
                </div>
                <div class="mini-metric">
                  <span>Ultrasonic</span>
                  <strong data-bind="distanceValidity">--</strong>
                </div>
              </div>
            </article>

            <article class="panel">
              <div class="panel-heading">
                <div>
                  <p class="eyebrow">Connectivity</p>
                  <h3>System access</h3>
                </div>
                <span class="panel-badge" data-bind="wifiState">--</span>
              </div>
              <div class="system-summary">
                <div>
                  <span>Connection</span>
                  <strong data-bind="wifiSummary">--</strong>
                </div>
                <div>
                  <span>PIKU access point</span>
                  <strong data-bind="softApIp">--</strong>
                </div>
                <div>
                  <span>Uptime</span>
                  <strong data-bind="uptime">--</strong>
                </div>
                <div>
                  <span>Firmware</span>
                  <strong data-bind="firmware">--</strong>
                </div>
              </div>
            </article>
          </div>

          <article class="panel quick-panel">
            <div class="panel-heading">
              <div>
                <p class="eyebrow">Quick access</p>
                <h3>Mission tools</h3>
              </div>
            </div>
            <div class="quick-links">
              <button class="quick-link" type="button" data-section="control">
                <strong>Control &amp; Radar</strong>
                <span>Drive controls and live scan</span>
              </button>
              <button class="quick-link" type="button" data-section="sensors">
                <strong>Sensors</strong>
                <span>Detailed sensor readings</span>
              </button>
              <button class="quick-link" type="button" data-section="communication">
                <strong>Communication</strong>
                <span>Wi-Fi, SoftAP and nRF24</span>
              </button>
            </div>
          </article>
        </section>

        <section class="dashboard-section" id="control" role="tabpanel" aria-label="Control and radar" hidden>
          <div class="section-heading">
            <div>
              <p class="eyebrow">Operations</p>
              <h2>Control &amp; Radar</h2>
            </div>
            <span class="section-status" data-bind="navigationDecision">--</span>
          </div>

          <div class="control-layout">
            <div class="control-column">
              <article class="panel control-panel">
                <div class="panel-heading">
                  <div>
                    <p class="eyebrow">Drive mode</p>
                    <h3>Robot control</h3>
                  </div>
                  <span class="panel-badge" id="mode">--</span>
                </div>

                <div class="mode-bar">
                  <button class="mode-btn" id="autoBtn" type="button" onclick="setMode('AUTO')">AUTO MODE</button>
                  <button class="mode-btn" id="manualBtn" type="button" onclick="setMode('MANUAL')">MANUAL MODE</button>
                </div>
                <div class="turn-bar">
                  <button class="turn-btn" id="turnModeBtn" type="button" onclick="toggleTurnMode()">Turn Mode: PIVOT</button>
                </div>

                <div class="speed-control">
                  <label for="motorSpeed">Motor speed <strong id="speedPercent">60%</strong></label>
                  <div class="speed-range">
                    <span>0%</span>
                    <input id="motorSpeed" type="range" min="0" max="100" step="1" value="60"
                           aria-label="Motor speed percent" oninput="previewSpeed(this.value)">
                    <span>100%</span>
                  </div>
                </div>
                <div class="control-readouts">
                  <div><span>Movement</span><strong id="motor">--</strong></div>
                  <div><span>Navigation</span><strong id="nav">--</strong></div>
                  <div><span>Turn mode</span><strong id="turnMode">--</strong></div>
                  <div><span>Control RTT</span><strong id="controlRtt">--</strong></div>
                  <div><span>Command age</span><strong id="controlCommandAge">--</strong></div>
                </div>

                <div class="control-readouts" aria-label="AUTO navigation diagnostics">
                  <div><span>AUTO state</span><strong id="autoNavState">--</strong></div>
                  <div><span>IR zone / source</span><strong id="autoIrSource">--</strong></div>
                  <div><span>Relative spin</span><strong id="autoTurnAngle">--</strong></div>
                  <div><span>Preferred / selected</span><strong id="autoSelectedPath">--</strong></div>
                  <div><span>Spin result</span><strong id="autoTurnResult">--</strong></div>
                  <div><span>Recovery / blocked</span><strong id="autoBlockedRetry">--</strong></div>
                  <div><span>Scan L / F / R</span><strong id="autoScanSummary">--</strong></div>
                  <div><span>Hazard recovery</span><strong id="autoHazardRecovery">--</strong></div>
                  <div><span>Stuck recovery</span><strong id="autoStuckRecovery">--</strong></div>
                </div>

                <p class="control-note" id="controlAvailability">Manual controls are unavailable while AUTO mode is active.</p>
                <div class="control-readouts">
                  <button id="overrideBtn" type="button" disabled>Enable manual flame/gas override</button>
                  <strong id="overrideStatus">Override OFF</strong>
                </div>
                <p id="overrideAlert" class="control-note" role="alert" hidden>HAZARD ACTIVE ? MANUAL OVERRIDE. Flame/gas alarm remains active.</p>
                <p class="control-note">Manual Reverse is operator-supervised; rear clearance is NOT sensor-verified.</p>
                <div class="dpad auto" id="dpad" aria-label="Manual directional controls">
                  <button class="forward hold" type="button" data-move="FORWARD" aria-label="Move forward">
                    <span class="arrow arrow-up" aria-hidden="true"></span>
                    <span>Forward</span>
                  </button>
                  <button class="left hold" type="button" data-move="LEFT" aria-label="Turn left">
                    <span class="arrow arrow-left" aria-hidden="true"></span>
                    <span>Left</span>
                  </button>
                  <button class="stop" id="stopBtn" type="button" aria-label="Manual stop">
                    <strong>STOP</strong>
                    <span>Manual</span>
                  </button>
                  <button class="right hold" type="button" data-move="RIGHT" aria-label="Turn right">
                    <span class="arrow arrow-right" aria-hidden="true"></span>
                    <span>Right</span>
                  </button>
                  <button class="backward hold" type="button" data-move="BACKWARD" aria-label="Move backward">
                    <span class="arrow arrow-down" aria-hidden="true"></span>
                    <span>Backward</span>
                  </button>
                </div>

                <div class="sensor-head-control" aria-labelledby="sensorHeadTitle">
                  <div class="sensor-head-heading">
                    <div>
                      <p class="eyebrow">Focused ultrasonic inspection</p>
                      <h4 id="sensorHeadTitle">SENSOR HEAD CONTROL</h4>
                    </div>
                    <span id="sensorHeadOrientation">FRONT</span>
                  </div>
                  <div class="sensor-head-buttons">
                    <button class="sensor-head-btn" id="sensorHeadLeftBtn" type="button" data-sensor-head="LEFT" onclick="requestSensorHead('LEFT')">LOOK LEFT</button>
                    <button class="sensor-head-btn" id="sensorHeadFrontBtn" type="button" data-sensor-head="FRONT" onclick="requestSensorHead('FRONT')">LOOK FRONT</button>
                    <button class="sensor-head-btn" id="sensorHeadRightBtn" type="button" data-sensor-head="RIGHT" onclick="requestSensorHead('RIGHT')">LOOK RIGHT</button>
                  </div>
                  <p class="sensor-head-availability" id="sensorHeadAvailability">Available in Manual mode</p>
                  <p class="sensor-head-status" id="sensorHeadStatus" role="status" aria-live="polite">Manual mode required</p>
                </div>
              </article>
            </div>

            <div class="radar-column">
              <article class="panel radar-card">
                <div class="panel-heading">
                  <div>
                    <p class="eyebrow">Ultrasonic scan</p>
                    <h3>Live radar</h3>
                  </div>
                  <span class="panel-badge" data-bind="distanceValidity">--</span>
                </div>
                <canvas id="radar" width="320" height="190" aria-label="Live ultrasonic radar"></canvas>
                <div class="radar-readout" id="radarReadout">Ultrasonic scan: --</div>
              </article>

              <article class="panel">
                <div class="detail-grid compact-details">
                  <div class="detail-item"><span>Current distance</span><strong id="dist">--</strong></div>
                  <div class="detail-item"><span>Servo scan</span><strong id="scan">--</strong></div>
                  <div class="detail-item"><span>Servo angle</span><strong data-bind="servoAngle">--</strong></div>
                  <div class="detail-item"><span>Scan orientation</span><strong data-bind="scanOrientation">--</strong></div>
                  <div class="detail-item"><span>Ultrasonic state</span><strong data-bind="ultrasonicState">--</strong></div>
                  <div class="detail-item"><span>Near obstacle</span><strong data-bind="nearObstacleDetail">--</strong></div>
                </div>
              </article>
            </div>
          </div>
        </section>

        <section class="dashboard-section" id="sensors" role="tabpanel" aria-label="Sensors" hidden>
          <div class="section-heading">
            <div>
              <p class="eyebrow">Stage-2 live data</p>
              <h2>Sensors &amp; Robot State</h2>
            </div>
            <span class="section-status" data-bind="safetyState">--</span>
          </div>

          <div class="sensor-grid">
            <article class="panel sensor-panel">
              <div class="panel-heading">
                <div>
                  <p class="eyebrow">Fast close-range inputs</p>
                  <h3>Dual front IR</h3>
                </div>
              </div>
              <div class="large-reading-grid">
                <div class="large-reading">
                  <span>Left obstacle</span>
                  <strong id="leftIrObstacle">--</strong>
                </div>
                <div class="large-reading">
                  <span>Right obstacle</span>
                  <strong id="rightIrObstacle">--</strong>
                </div>
              </div>
            </article>

            <article class="panel sensor-panel">
              <div class="panel-heading">
                <div>
                  <p class="eyebrow">Digital input diagnostics</p>
                  <h3>IR raw levels</h3>
                </div>
                <span class="panel-badge">CONFIGURABLE POLARITY</span>
              </div>
              <div class="detail-grid">
                <div class="detail-item"><span>Left IR raw</span><strong id="leftIrRaw">--</strong></div>
                <div class="detail-item"><span>Right IR raw</span><strong id="rightIrRaw">--</strong></div>
                <div class="detail-item"><span>Combined obstacle</span><strong id="irObstacle">--</strong></div>
              </div>
            </article>

            <article class="panel sensor-panel">
              <div class="panel-heading">
                <div>
                  <p class="eyebrow">Range and fusion</p>
                  <h3>Obstacle assessment</h3>
                </div>
              </div>
              <div class="detail-grid">
                <div class="detail-item"><span>Near obstacle</span><strong id="nearObstacle">--</strong></div>
                <div class="detail-item"><span>Near source</span><strong id="nearSource">--</strong></div>
                <div class="detail-item"><span>Distance</span><strong id="sensorDistance">--</strong></div>
                <div class="detail-item"><span>Servo direction</span><strong id="sensorServo">--</strong></div>
              </div>
            </article>

            <article class="panel sensor-panel">
              <div class="panel-heading">
                <div>
                  <p class="eyebrow">Safety classification</p>
                  <h3>System assessment</h3>
                </div>
              </div>
              <div class="detail-grid">
                <div class="detail-item"><span>Safety state</span><strong id="safety">--</strong></div>
                <div class="detail-item"><span>Primary alarm</span><strong id="alarmReason">--</strong></div>
                <div class="detail-item"><span>Ultrasonic warning</span><strong id="ultraWarn">--</strong></div>
                <div class="detail-item"><span>Ultrasonic state</span><strong id="ultraState">--</strong></div>
                <div class="detail-item full"><span>Ultrasonic diagnostics</span><strong id="ultraDiag">--</strong></div>
              </div>
            </article>

            <article class="panel sensor-panel">
              <div class="panel-heading"><div><p class="eyebrow">Environmental sensing</p><h3>DHT22, MQ-2 &amp; flame DO</h3></div><span class="panel-badge" data-bind="stage2Status">ACTIVE</span></div>
              <div class="detail-grid">
                <div class="detail-item"><span>Temperature</span><strong id="envTemp">--</strong></div>
                <div class="detail-item"><span>Humidity</span><strong id="envHum">--</strong></div>
                <div class="detail-item"><span>MQ-2 raw</span><strong id="envGasRaw">--</strong></div>
                <div class="detail-item"><span>MQ-2 filtered</span><strong id="envGasFiltered">--</strong></div>
                <div class="detail-item"><span>Gas state</span><strong id="envGasState">--</strong></div>
                <div class="detail-item"><span>Flame</span><strong id="envFlame">--</strong></div>
                <div class="detail-item full"><span>DHT22 diagnostics</span><strong id="envDhtDiag">--</strong></div>
                <div class="detail-item full"><span>MQ-2 timing</span><strong id="envMq2Diag">--</strong></div>
                <div class="detail-item full"><span>Flame DO diagnostics</span><strong id="envFlameDiag">--</strong></div>
              </div>
              <p class="system-note">MQ-2 values are qualitative relative ADC bands, not calibrated ppm. Flame sensing is the module's active-HIGH digital comparator output; it provides no distance or intensity measurement.</p>
            </article>

            <article class="panel sensor-panel">
              <div class="panel-heading"><div><p class="eyebrow">Robot-state awareness</p><h3>MPU6050 IMU</h3></div><span class="panel-badge" id="imuStatus">--</span></div>
              <div class="detail-grid">
                <div class="detail-item"><span>Roll</span><strong id="imuRoll">--</strong></div>
                <div class="detail-item"><span>Pitch</span><strong id="imuPitch">--</strong></div>
                <div class="detail-item full"><span>Acceleration X / Y / Z</span><strong id="imuAccel">--</strong></div>
                <div class="detail-item full"><span>Gyro X / Y / Z</span><strong id="imuGyro">--</strong></div>
                <div class="detail-item"><span>Motion</span><strong id="imuMotion">--</strong></div>
                <div class="detail-item"><span>Sample age</span><strong id="imuAge">--</strong></div>
                <div class="detail-item full"><span>Health diagnostics</span><strong id="imuDiag">--</strong></div>
              </div>
            </article>

          </div>
        </section>

        <section class="dashboard-section" id="communication" role="tabpanel" aria-label="Communication" hidden>
          <div class="section-heading">
            <div>
              <p class="eyebrow">Recovery and links</p>
              <h2>Communication</h2>
            </div>
            <span class="product-chip">PIKU 2.1 Rev A</span>
          </div>

          <div class="system-layout">
            <article class="panel">
              <div class="panel-heading">
                <div>
                  <p class="eyebrow">Always available</p>
                  <h3>PIKU access point</h3>
                </div>
                <span class="panel-badge" id="apStatus">--</span>
              </div>
              <div class="detail-grid">
                <div class="detail-item"><span>SoftAP IP</span><strong id="apIp">--</strong></div>
                <div class="detail-item"><span>Channel</span><strong id="apChannel">--</strong></div>
                <div class="detail-item"><span>Connected clients</span><strong id="apClients">--</strong></div>
                <div class="detail-item"><span>Local hostname</span><strong id="localHostname">--</strong></div>
                <div class="detail-item full"><span>Access URL</span><strong id="apUrl">--</strong></div>
              </div>
            </article>

            <article class="panel">
              <div class="panel-heading">
                <div>
                  <p class="eyebrow">External network</p>
                  <h3>Home Wi-Fi</h3>
                </div>
                <span class="panel-badge" id="staStatus">--</span>
              </div>
              <div class="detail-grid">
                <div class="detail-item"><span>Configured</span><strong id="staConfiguredStatus">--</strong></div>
                <div class="detail-item"><span>Saved SSID</span><strong id="savedSsid">--</strong></div>
                <div class="detail-item"><span>STA IP</span><strong id="staIp">--</strong></div>
                <div class="detail-item"><span>RSSI</span><strong id="staRssi">--</strong></div>
                <div class="detail-item full"><span>Home Wi-Fi URL</span><strong id="staUrl">--</strong></div>
                <div class="detail-item full"><span>mDNS access</span><strong id="mdnsUrl">--</strong></div>
                <div class="detail-item full"><span>mDNS status</span><strong id="mdnsStatus">--</strong></div>
              </div>
              <div class="wifi-gauge" aria-label="Wi-Fi signal quality"><span id="wifiGaugeFill"></span></div>
            </article>

            <article class="panel wifi-panel">
              <div class="panel-heading">
                <div>
                  <p class="eyebrow">Provisioning</p>
                  <h3>Connect home Wi-Fi</h3>
                </div>
              </div>
              <div class="wifi-form">
                <button class="wifi-btn wifi-provision-action" id="wifiScanBtn" type="button" onclick="scanWifi(event)">Scan networks</button>
                <label for="wifiNetworks">Available networks</label>
                <select id="wifiNetworks" onchange="selectWifiNetwork()">
                  <option value="">Select network</option>
                </select>
                <label for="wifiSsid">SSID</label>
                <input id="wifiSsid" maxlength="32" placeholder="Network name" autocomplete="off">
                <label for="wifiPassword">Password</label>
                <input id="wifiPassword" maxlength="64" placeholder="Network password" type="password" autocomplete="current-password">
                <div class="wifi-actions">
                  <button class="wifi-btn wifi-provision-action" id="wifiConnectBtn" type="button" onclick="connectWifi(event)">Connect</button>
                  <button class="wifi-btn wifi-provision-action" id="wifiReconnectBtn" type="button" onclick="reconnectWifi(event)">Reconnect</button>
                  <button class="wifi-btn danger wifi-provision-action" id="wifiForgetBtn" type="button" onclick="forgetWifi(event)">Forget</button>
                </div>
                <div class="wifi-note" id="wifiProvisionStatus">PIKU Wi-Fi remains available while Home Wi-Fi is configured.</div>
              </div>
            </article>

            <article class="panel">
              <div class="panel-heading">
                <div><p class="eyebrow">Shared-SPI radio</p><h3>nRF24 link</h3></div>
                <span class="panel-badge" id="commRadioHardware">--</span>
              </div>
              <div class="detail-grid">
                <div class="detail-item"><span>Enabled</span><strong id="commRadioEnabled">--</strong></div>
                <div class="detail-item"><span>Peer</span><strong id="commRadioPeer">--</strong></div>
                <div class="detail-item"><span>Heartbeat / last TX</span><strong id="commRadioHeartbeat">--</strong></div>
                <div class="detail-item"><span>Last RX</span><strong id="commRadioRxAge">--</strong></div>
                <div class="detail-item"><span>TX success / fail</span><strong id="commRadioTx">--</strong></div>
                <div class="detail-item"><span>RX packets</span><strong id="commRadioRx">--</strong></div>
              </div>
              <p class="system-note">Missing-peer traffic backs off; standalone sensing and control remain available.</p>
            </article>

            <article class="panel alarm-panel">
              <div class="panel-heading">
                <div>
                  <p class="eyebrow">Audible warning</p>
                  <h3>Alarm controls</h3>
                </div>
                <span class="panel-badge" id="alarmSound">--</span>
              </div>
              <div class="detail-grid">
                <div class="detail-item"><span>Physical buzzer</span><strong id="buzzer">--</strong></div>
                <div class="detail-item"><span>Browser audio</span><strong id="audioStatus">AUDIO DISABLED</strong></div>
                <div class="detail-item"><span>Operating mode</span><strong data-bind="mode">--</strong></div>
                <div class="detail-item"><span>Turn mode</span><strong data-bind="turnMode">--</strong></div>
              </div>
              <div class="alarm-bar">
                <button class="alarm-btn" id="enableAlarmBtn" type="button" onclick="enableAlarm(event)">Enable alarm</button>
                <button class="alarm-btn" id="testBeepBtn" type="button" onclick="testBeep(event)">Test beep</button>
                <button class="alarm-btn danger" id="muteAlarmBtn" type="button" onclick="disableAlarm(event)">Mute alarm</button>
              </div>
              <p class="system-note">Browser audio requires a user action. Muting sound never hides visual warnings.</p>
            </article>
          </div>
        </section>

        <section class="dashboard-section" id="system" role="tabpanel" aria-label="System" hidden>
          <div class="section-heading">
            <div><p class="eyebrow">Engineering status</p><h2>System diagnostics</h2></div>
            <span class="product-chip">PIKU 2.1 Rev A</span>
          </div>
          <div class="system-layout">
            <article class="panel">
              <div class="panel-heading"><div><p class="eyebrow">Runtime</p><h3>Firmware and memory</h3></div></div>
              <div class="detail-grid">
                <div class="detail-item"><span>Firmware</span><strong id="systemFirmware">--</strong></div>
                <div class="detail-item"><span>Uptime</span><strong id="systemUptime">--</strong></div>
                <div class="detail-item"><span>Free heap</span><strong id="systemFreeHeap">--</strong></div>
                <div class="detail-item"><span>Minimum free heap</span><strong id="systemMinHeap">--</strong></div>
                <div class="detail-item"><span>PSRAM</span><strong id="systemPsram">--</strong></div>
                <div class="detail-item"><span>Reset reason</span><strong id="systemResetReason">--</strong></div>
                <div class="detail-item"><span>Local OLED</span><strong id="displayState">--</strong></div>
              </div>
            </article>
            <article class="panel">
              <div class="panel-heading"><div><p class="eyebrow">Control path</p><h3>Timing and commands</h3></div></div>
              <div class="detail-grid">
                <div class="detail-item"><span>Maximum service-loop gap</span><strong id="systemLoopGap">--</strong></div>
                <div class="detail-item"><span>HTTP errors</span><strong id="systemHttpErrors">--</strong></div>
                <div class="detail-item"><span>HTTP client timeouts</span><strong id="systemHttpTimeouts">--</strong></div>
                <div class="detail-item"><span>Browser telemetry timeouts</span><strong id="systemTelemetryTimeouts">0</strong></div>
                <div class="detail-item"><span>Wi-Fi reconnects</span><strong id="systemWifiReconnects">--</strong></div>
                <div class="detail-item"><span>Last accepted command</span><strong id="systemLastAccepted">--</strong></div>
                <div class="detail-item"><span>Last applied command</span><strong id="systemLastApplied">--</strong></div>
                <div class="detail-item"><span>Sequence</span><strong id="systemCommandSequence">--</strong></div>
                <div class="detail-item"><span>Last STOP reason</span><strong id="systemStopReason">--</strong></div>
                <div class="detail-item"><span>Command age</span><strong id="systemCommandAge">--</strong></div>
                <div class="detail-item"><span>Handler-to-apply</span><strong id="systemApplyLag">--</strong></div>
              </div>
              <p class="system-note">Timing values are internal software diagnostics, not measured phone-to-motor latency.</p>
            </article>
            <article class="panel">
              <div class="panel-heading"><div><p class="eyebrow">Health counters</p><h3>Sensor summary</h3></div></div>
              <div class="detail-grid">
                <div class="detail-item"><span>DHT errors</span><strong id="systemDhtErrors">--</strong></div>
                <div class="detail-item"><span>Ultrasonic faults</span><strong id="systemUltrasonicFaults">--</strong></div>
                <div class="detail-item"><span>IMU errors</span><strong id="systemImuErrors">--</strong></div>
                <div class="detail-item"><span>nRF TX failures</span><strong id="systemRadioFailures">--</strong></div>
              </div>
            </article>
          </div>
        </section>
      </main>
    </div>
    <script src="/dashboard.js"></script>
  </body>
</html>
)html";

const char DASHBOARD_CSS[] PROGMEM = R"css(
:root {
  color-scheme: dark;
  --bg:#07111f;
  --bg-deep:#030914;
  --panel:#0d1b2b;
  --panel-strong:#12243a;
  --line:#20364d;
  --line-soft:#172b40;
  --text:#edf7ff;
  --muted:#91a7bb;
  --cyan:#36d8ff;
  --cyan-soft:#a8efff;
  --blue:#1f78ff;
  --green:#22c983;
  --amber:#f5b942;
  --red:#f05252;
  --red-strong:#d92f45;
  --violet:#8b6cf6;
  --shadow:0 16px 42px rgba(0,0,0,.28);
}
* { box-sizing:border-box; }
html { min-height:100%; background:var(--bg-deep); }
body {
  min-height:100vh;
  margin:0;
  background:
    radial-gradient(circle at 15% 0%, rgba(31,120,255,.12), transparent 34rem),
    linear-gradient(180deg, var(--bg-deep), var(--bg));
  color:var(--text);
  font-family:Inter, "Segoe UI", Arial, sans-serif;
  line-height:1.45;
  overflow-x:hidden;
}
button, input, select { font:inherit; }
button { color:inherit; }
button:focus-visible, input:focus-visible, select:focus-visible {
  outline:3px solid rgba(54,216,255,.5);
  outline-offset:2px;
}
.app-shell { width:min(1180px, 100%); margin:0 auto; padding:0 18px 52px; }
.app-header {
  position:sticky;
  top:0;
  z-index:20;
  padding:14px 0 0;
  background:linear-gradient(180deg, rgba(3,9,20,.98) 72%, rgba(3,9,20,.88) 90%, transparent);
  backdrop-filter:blur(12px);
}
.header-main {
  display:flex;
  align-items:center;
  justify-content:space-between;
  gap:18px;
  padding:12px 16px;
  border:1px solid var(--line);
  border-radius:14px 14px 0 0;
  background:rgba(13,27,43,.94);
}
.brand { display:flex; align-items:center; gap:12px; min-width:0; }
.brand-mark {
  display:grid;
  place-items:center;
  flex:0 0 42px;
  height:42px;
  border:1px solid rgba(54,216,255,.5);
  border-radius:11px;
  background:linear-gradient(145deg, rgba(54,216,255,.2), rgba(31,120,255,.14));
  color:var(--cyan);
  font-size:22px;
  font-weight:900;
}
h1, h2, h3, p { margin-top:0; }
h1 { margin-bottom:1px; font-size:20px; letter-spacing:.025em; }
.brand p { margin:0; color:var(--muted); font-size:12px; }
.header-status { display:flex; align-items:center; justify-content:flex-end; gap:8px; flex-wrap:wrap; }
.status-badge, .panel-badge, .section-status, .product-chip {
  display:inline-flex;
  align-items:center;
  min-height:32px;
  padding:6px 10px;
  border:1px solid var(--line);
  border-radius:999px;
  background:#0a1726;
  color:var(--muted);
  font-size:12px;
  font-weight:700;
  white-space:nowrap;
}
.mode-badge strong { margin-left:5px; color:var(--cyan-soft); }
.connection-badge { gap:7px; }
.connection-dot { width:8px; height:8px; border-radius:50%; background:var(--amber); box-shadow:0 0 0 4px rgba(245,185,66,.12); }
.connection-badge.is-live { color:#b8f8d9; border-color:rgba(34,201,131,.38); }
.connection-badge.is-live .connection-dot { background:var(--green); box-shadow:0 0 0 4px rgba(34,201,131,.12); }
.connection-badge.is-offline { color:#ffd0d0; border-color:rgba(240,82,82,.45); }
.connection-badge.is-offline .connection-dot { background:var(--red); box-shadow:0 0 0 4px rgba(240,82,82,.13); }
.connection-badge.is-stale { color:#ffe3a3; border-color:rgba(245,185,66,.48); }
.connection-badge.is-stale .connection-dot { background:var(--amber); }
.alarm-indicator {
  display:flex;
  align-items:center;
  gap:12px;
  min-height:47px;
  padding:10px 16px;
  border:1px solid rgba(34,201,131,.42);
  border-top:0;
  border-radius:0 0 14px 14px;
  background:linear-gradient(90deg, rgba(12,90,66,.92), rgba(11,64,56,.9));
  box-shadow:var(--shadow);
}
.alarm-label { flex:0 0 auto; font-size:13px; font-weight:900; letter-spacing:.055em; }
.alarm-detail { min-width:0; color:#d8fff0; font-size:13px; overflow-wrap:anywhere; }
.alarm-indicator.caution {
  border-color:rgba(245,185,66,.55);
  background:linear-gradient(90deg, rgba(128,82,15,.95), rgba(86,58,18,.93));
}
.alarm-indicator.critical, .alarm-indicator.high, .alarm-indicator.near {
  border-color:rgba(255,159,159,.65);
  background:linear-gradient(90deg, rgba(157,35,51,.97), rgba(106,25,39,.95));
}
.alarm-indicator.fault {
  border-color:rgba(139,108,246,.62);
  background:linear-gradient(90deg, rgba(75,54,151,.96), rgba(54,40,111,.95));
}
.section-nav {
  position:sticky;
  top:128px;
  z-index:15;
  display:grid;
  grid-template-columns:repeat(5, minmax(0,1fr));
  gap:7px;
  margin:12px 0 22px;
  padding:7px;
  border:1px solid var(--line);
  border-radius:12px;
  background:rgba(7,17,31,.94);
  backdrop-filter:blur(12px);
}
.nav-btn {
  min-height:42px;
  border:1px solid transparent;
  border-radius:8px;
  background:transparent;
  color:var(--muted);
  font-weight:800;
  cursor:pointer;
}
.nav-btn:hover { color:var(--text); background:rgba(31,120,255,.09); }
.nav-btn.active {
  border-color:rgba(54,216,255,.3);
  background:linear-gradient(135deg, rgba(31,120,255,.27), rgba(54,216,255,.1));
  color:var(--cyan-soft);
  box-shadow:inset 0 0 0 1px rgba(54,216,255,.08);
}
.dashboard-section[hidden] { display:none !important; }
.dashboard-section.active { display:block; }
.section-heading {
  display:flex;
  align-items:flex-end;
  justify-content:space-between;
  gap:16px;
  margin-bottom:14px;
}
.section-heading h2 { margin:0; font-size:clamp(23px, 4vw, 32px); letter-spacing:-.025em; }
.eyebrow {
  margin-bottom:4px;
  color:var(--cyan);
  font-size:11px;
  font-weight:900;
  letter-spacing:.13em;
  text-transform:uppercase;
}
.section-status, .product-chip { color:var(--cyan-soft); }
.panel, .metric-card {
  border:1px solid var(--line);
  border-radius:14px;
  background:linear-gradient(150deg, rgba(18,36,58,.96), rgba(10,24,39,.97));
  box-shadow:var(--shadow);
}
.panel { padding:16px; }
.panel-heading {
  display:flex;
  align-items:flex-start;
  justify-content:space-between;
  gap:12px;
  margin-bottom:14px;
}
.panel-heading h3 { margin:0; font-size:17px; }
.panel-badge { min-height:29px; padding:5px 9px; color:var(--cyan-soft); }
.summary-grid { display:grid; gap:12px; }
.summary-grid-primary { grid-template-columns:repeat(4, minmax(0,1fr)); }
.metric-card { min-height:142px; padding:16px; display:flex; flex-direction:column; }
.metric-label { color:var(--muted); font-size:12px; font-weight:700; }
.metric-value { margin:14px 0 8px; color:var(--text); font-size:25px; line-height:1.05; overflow-wrap:anywhere; }
.metric-note { margin-top:auto; color:var(--cyan-soft); font-size:12px; overflow-wrap:anywhere; }
.safety-card { border-color:rgba(34,201,131,.34); }
.overview-layout { display:grid; grid-template-columns:1.35fr .85fr; gap:14px; margin-top:14px; }
.mini-metric-grid { display:grid; grid-template-columns:repeat(3,minmax(0,1fr)); gap:9px; }
.mini-metric, .system-summary > div, .detail-item, .control-readouts > div {
  min-width:0;
  padding:11px;
  border:1px solid var(--line-soft);
  border-radius:10px;
  background:rgba(3,12,23,.42);
}
.mini-metric span, .system-summary span, .detail-item span, .control-readouts span, .large-reading span {
  display:block;
  margin-bottom:5px;
  color:var(--muted);
  font-size:11px;
  font-weight:700;
}
.mini-metric strong, .system-summary strong, .detail-item strong, .control-readouts strong {
  display:block;
  color:var(--text);
  font-size:14px;
  overflow-wrap:anywhere;
}
.system-summary { display:grid; gap:9px; }
.quick-panel { margin-top:14px; }
.quick-links { display:grid; grid-template-columns:repeat(3,minmax(0,1fr)); gap:10px; }
.quick-link {
  min-height:82px;
  padding:13px;
  border:1px solid var(--line);
  border-radius:10px;
  background:#0a1828;
  text-align:left;
  cursor:pointer;
}
.quick-link:hover { border-color:rgba(54,216,255,.46); background:#0d2034; }
.quick-link strong, .quick-link span { display:block; }
.quick-link strong { margin-bottom:5px; color:var(--cyan-soft); }
.quick-link span { color:var(--muted); font-size:12px; }
.secondary-btn, .mode-btn, .turn-btn, .wifi-btn, .alarm-btn {
  border:1px solid var(--line);
  border-radius:9px;
  background:#142c46;
  font-weight:800;
  cursor:pointer;
}
.secondary-btn:hover, .mode-btn:hover, .turn-btn:hover, .wifi-btn:hover, .alarm-btn:hover { border-color:rgba(54,216,255,.55); }
.compact-btn { min-height:38px; padding:7px 12px; font-size:12px; }
.control-layout { display:grid; grid-template-columns:minmax(330px,.88fr) minmax(390px,1.12fr); gap:14px; }
.control-column, .radar-column { min-width:0; }
.control-panel { height:100%; }
.mode-bar { display:grid; grid-template-columns:1fr 1fr; gap:9px; }
.mode-btn { min-height:50px; color:var(--muted); }
.mode-btn.active { border-color:rgba(54,216,255,.62); background:#12677f; color:white; box-shadow:inset 0 0 0 1px rgba(168,239,255,.18); }
.turn-bar { margin:9px 0; }
.turn-btn { width:100%; min-height:44px; color:var(--text); }
.turn-btn.spin { border-color:rgba(139,108,246,.6); background:#5437aa; }
.turn-btn:disabled { opacity:.48; cursor:not-allowed; }
.speed-control {
  margin:9px 0 12px;
  padding:11px 12px;
  border:1px solid var(--line-soft);
  border-radius:10px;
  background:rgba(3,12,23,.42);
}
.speed-control label { display:flex; justify-content:space-between; gap:12px; color:var(--muted); font-size:12px; font-weight:800; }
.speed-control strong { color:var(--cyan); font-size:15px; }
.speed-range { display:grid; grid-template-columns:auto minmax(0,1fr) auto; align-items:center; gap:10px; margin-top:8px; color:var(--muted); font-size:11px; }
.speed-range input { width:100%; accent-color:var(--cyan); cursor:pointer; }
.control-readouts { display:grid; grid-template-columns:repeat(3,minmax(0,1fr)); gap:8px; margin:12px 0; }
.control-note {
  min-height:20px;
  margin:4px 0 11px;
  color:var(--amber);
  font-size:12px;
  text-align:center;
}
.dpad {
  display:grid;
  grid-template-columns:repeat(3,minmax(0,1fr));
  grid-template-areas:". forward ." "left stop right" ". backward .";
  gap:10px;
  max-width:430px;
  margin:0 auto;
  user-select:none;
  -webkit-user-select:none;
  -webkit-touch-callout:none;
}
.dpad button {
  display:flex;
  flex-direction:column;
  align-items:center;
  justify-content:center;
  gap:7px;
  min-height:72px;
  padding:8px;
  border:1px solid rgba(79,151,255,.55);
  border-radius:11px;
  background:linear-gradient(150deg,#185fc5,#12488f);
  color:white;
  font-weight:900;
  touch-action:none;
  -ms-touch-action:none;
  cursor:pointer;
}
.dpad button:active:not(:disabled) { transform:translateY(1px); background:#0e53b5; }
.dpad .stop {
  grid-area:stop;
  min-height:86px;
  border-color:rgba(255,161,161,.58);
  background:linear-gradient(150deg,var(--red),var(--red-strong));
}
.dpad .stop strong { font-size:21px; letter-spacing:.06em; }
.dpad .stop span { font-size:11px; }
.forward { grid-area:forward; }
.backward { grid-area:backward; }
.left { grid-area:left; }
.right { grid-area:right; }
.dpad.auto .hold, .dpad button:disabled {
  opacity:.32;
  filter:grayscale(.75);
  cursor:not-allowed;
  transform:none;
}
.sensor-head-control {
  margin-top:14px;
  padding:12px;
  border:1px solid var(--line-soft);
  border-radius:11px;
  background:rgba(3,12,23,.42);
}
.sensor-head-heading {
  display:flex;
  align-items:center;
  justify-content:space-between;
  gap:12px;
  margin-bottom:10px;
}
.sensor-head-heading h4 { margin:0; color:var(--text); font-size:13px; letter-spacing:.04em; }
.sensor-head-heading .eyebrow { margin-bottom:3px; }
#sensorHeadOrientation {
  padding:5px 8px;
  border:1px solid var(--line);
  border-radius:999px;
  color:var(--cyan-soft);
  font-size:11px;
  font-weight:900;
}
.sensor-head-buttons { display:grid; grid-template-columns:repeat(3,minmax(0,1fr)); gap:8px; }
.sensor-head-btn {
  min-height:42px;
  padding:7px 5px;
  border:1px solid rgba(79,151,255,.48);
  border-radius:9px;
  background:#142c46;
  color:var(--text);
  font-size:11px;
  font-weight:900;
  cursor:pointer;
}
.sensor-head-btn.active {
  border-color:rgba(54,216,255,.72);
  background:#12677f;
  color:white;
  box-shadow:inset 0 0 0 1px rgba(168,239,255,.18);
}
.sensor-head-btn:disabled { opacity:.36; filter:grayscale(.7); cursor:not-allowed; }
.sensor-head-availability, .sensor-head-status {
  margin:8px 0 0;
  color:var(--muted);
  font-size:11px;
  text-align:center;
}
.sensor-head-status { color:var(--cyan-soft); }
.sensor-head-status.warning { color:#fca5a5; font-weight:800; }
.arrow { display:block; width:0; height:0; }
.arrow-up { border-left:8px solid transparent; border-right:8px solid transparent; border-bottom:12px solid currentColor; }
.arrow-down { border-left:8px solid transparent; border-right:8px solid transparent; border-top:12px solid currentColor; }
.arrow-left { border-top:8px solid transparent; border-bottom:8px solid transparent; border-right:12px solid currentColor; }
.arrow-right { border-top:8px solid transparent; border-bottom:8px solid transparent; border-left:12px solid currentColor; }
.radar-card { margin-bottom:14px; }
#radar {
  display:block;
  width:100%;
  max-width:520px;
  height:auto;
  margin:0 auto;
  border:1px solid #17536a;
  border-radius:11px;
  background:#031114;
}
.radar-readout {
  margin-top:9px;
  padding:9px;
  border-radius:8px;
  background:rgba(3,17,20,.75);
  color:var(--cyan-soft);
  font-size:12px;
  font-weight:800;
  text-align:center;
}
.detail-grid { display:grid; grid-template-columns:repeat(2,minmax(0,1fr)); gap:9px; }
.detail-item.full { grid-column:1 / -1; }
.compact-details { grid-template-columns:repeat(3,minmax(0,1fr)); }
.sensor-grid, .system-layout { display:grid; grid-template-columns:repeat(2,minmax(0,1fr)); gap:14px; }
.large-reading-grid { display:grid; grid-template-columns:1fr 1fr; gap:10px; }
.large-reading {
  padding:17px;
  border:1px solid var(--line-soft);
  border-radius:11px;
  background:rgba(3,12,23,.42);
}
.large-reading strong { color:var(--cyan-soft); font-size:28px; line-height:1.1; }
.wifi-gauge { height:10px; margin-top:12px; border-radius:999px; background:#07111f; overflow:hidden; }
.wifi-gauge span { display:block; width:0; height:100%; border-radius:inherit; background:linear-gradient(90deg,var(--blue),var(--green)); transition:width .25s ease; }
.wifi-form { display:grid; gap:8px; }
.wifi-form label { color:var(--muted); font-size:11px; font-weight:800; }
.wifi-form input, .wifi-form select {
  width:100%;
  min-height:44px;
  padding:0 11px;
  border:1px solid var(--line);
  border-radius:9px;
  background:#071523;
  color:var(--text);
}
.wifi-btn, .alarm-btn { min-height:44px; padding:8px 12px; background:#12677f; }
.wifi-actions { display:grid; grid-template-columns:repeat(3,minmax(0,1fr)); gap:8px; margin-top:2px; }
.wifi-provision-action:disabled { opacity:.38; cursor:not-allowed; filter:grayscale(.65); }
.danger { background:#7d2735 !important; border-color:rgba(240,82,82,.46) !important; }
.wifi-note, .system-note {
  margin:4px 0 0;
  padding:9px;
  border:1px solid var(--line-soft);
  border-radius:8px;
  background:rgba(3,12,23,.4);
  color:var(--muted);
  font-size:11px;
}
.alarm-bar { display:grid; grid-template-columns:repeat(3,minmax(0,1fr)); gap:8px; margin-top:12px; }
@media (max-width:900px) {
  .summary-grid-primary { grid-template-columns:repeat(2,minmax(0,1fr)); }
  .overview-layout, .control-layout { grid-template-columns:1fr; }
  .compact-details { grid-template-columns:repeat(2,minmax(0,1fr)); }
}
@media (max-width:680px) {
  .app-shell { padding:0 10px 88px; }
  .app-header { padding-top:8px; }
  .header-main { align-items:flex-start; padding:10px 12px; }
  .brand-mark { flex-basis:36px; height:36px; font-size:18px; }
  h1 { font-size:17px; }
  .brand p { font-size:10px; }
  .header-status { flex-direction:column; align-items:flex-end; gap:5px; }
  .status-badge { min-height:27px; padding:4px 8px; font-size:10px; }
  .alarm-indicator { align-items:flex-start; flex-direction:column; gap:2px; min-height:51px; padding:8px 12px; }
  .alarm-label { font-size:11px; }
  .alarm-detail { font-size:11px; }
  .section-nav {
    position:fixed;
    top:auto;
    right:10px;
    bottom:9px;
    left:10px;
    z-index:30;
    margin:0;
    box-shadow:0 12px 35px rgba(0,0,0,.48);
  }
  .nav-btn { min-height:48px; padding:4px 2px; font-size:9px; }
  .dashboard-main { padding-top:14px; }
  .section-heading { align-items:flex-start; }
  .section-heading h2 { font-size:24px; }
  .compact-btn { display:none; }
  .summary-grid-primary, .sensor-grid, .system-layout { grid-template-columns:1fr; }
  .metric-card { min-height:116px; }
  .metric-value { margin:10px 0 6px; font-size:22px; }
  .mini-metric-grid { grid-template-columns:repeat(2,minmax(0,1fr)); }
  .quick-links { grid-template-columns:1fr; }
  .control-readouts, .compact-details { grid-template-columns:1fr 1fr; }
  .panel { padding:13px; }
  .dpad { gap:8px; }
  .dpad button { min-height:68px; font-size:12px; }
  .dpad .stop { min-height:80px; }
}
@media (max-width:390px) {
  .brand p { display:none; }
  .mode-badge { display:none; }
  .detail-grid, .large-reading-grid { grid-template-columns:1fr; }
  .detail-item.full { grid-column:auto; }
  .control-readouts { grid-template-columns:1fr; }
  .alarm-bar, .wifi-actions { grid-template-columns:1fr; }
}
@media (prefers-reduced-motion:reduce) {
  *, *::before, *::after { scroll-behavior:auto !important; transition:none !important; }
}
)css";

const char DASHBOARD_JS[] PROGMEM = R"js(
let mode = 'AUTO';
let controlSessionToken = null;
let sessionRequestInFlight = false;
let turnMode = 'PIVOT';
let audioCtx = null;
let alarmOutput = null;
let alarmEnabled = false;
let alarmMuted = false;
let highRiskTimer = null;
let highRiskVibeTimer = null;
let lastCautionBeep = 0;
let activePress = false;
let activeMove = null;
let moveRefreshTimer = null;
const MOVE_REFRESH_MS = 250;
const TELEMETRY_INTERVAL_MS = 450;
const TELEMETRY_TIMEOUT_MS = 1200;
let commandSequence = 0;
let moveRequestInFlight = false;
let stopRequestsInFlight = 0;
let pendingMove = null;
let activePointerId = null;
let moveAbortController = null;
let dataAbortController = null;
let lastControlRttMs = null;
let telemetryTimeoutCount = 0;
const scanSamples = [];
let lastDistanceSampleSequence = 0;
let latestData = null;
let lastRadarDraw = 0;
let dashboardInitialized = false;
let telemetryTimer = null;
let dataRequestPromise = null;
let radarFrameStarted = false;
let dataRequestInFlight = false;
let lastTelemetrySuccess = 0;
let wifiScanTimer = null;
let wifiScanInFlight = false;
let sensorHeadRequestedOrientation = null;
let sensorHeadRequestSequence = 0;
let speedRequestTimer = null;
let requestedSpeedPercent = null;
let speedRequestSequence = 0;
const RADAR_MAX_CM = 300;
const RADAR_RINGS_CM = [75, 150, 225, 300];

function byId(id) {
  return document.getElementById(id);
}

function setText(id, value) {
  const element = byId(id);
  if (element) element.textContent = value;
}

function setBound(name, value) {
  document.querySelectorAll('[data-bind="' + name + '"]').forEach(element => {
    element.textContent = value;
  });
}

function previewSpeed(value) {
  const speed = Math.max(0, Math.min(100, Number(value) || 0));
  requestedSpeedPercent = speed;
  setText('speedPercent', speed + '%');
  if (speedRequestTimer) clearTimeout(speedRequestTimer);
  speedRequestTimer = setTimeout(() => {
    speedRequestTimer = null;
    const requestSequence = ++speedRequestSequence;
    fetch('/speed?set=' + encodeURIComponent(speed))
      .then(response => {
        if (!response.ok) throw new Error('Speed request rejected');
        if (requestSequence === speedRequestSequence && requestedSpeedPercent === speed) {
          requestedSpeedPercent = null;
        }
        return loadData();
      })
      .catch(() => {
        if (requestSequence === speedRequestSequence && requestedSpeedPercent === speed) {
          requestedSpeedPercent = null;
        }
        loadData();
      });
  }, 180);
}
function setMode(nextMode) {
  explicitStop();
  mode = nextMode;
  updateModeButtons();
  fetch('/mode?set=' + encodeURIComponent(nextMode), { cache:'no-store' })
    .then(response => {
      if (!response.ok) throw new Error('Mode request rejected');
      controlSessionToken = null;
      return loadData();
    })
    .catch(() => loadData());
}

function setTurnMode(nextMode) {
  fetch('/turn-mode?set=' + encodeURIComponent(nextMode)).then(() => loadData()).catch(() => {});
}

function toggleTurnMode() {
  if (mode !== 'MANUAL') return;
  setTurnMode(turnMode === 'PIVOT' ? 'SPIN' : 'PIVOT');
}

function sendCmd(move) {
  if (move === 'STOP' || move === 'RELEASE') {
    // STOP bypasses a slow direction request. The higher sequence makes any
    // older direction harmless if it reaches the ESP32 afterward.
    pendingMove = null;
    if (moveAbortController) moveAbortController.abort();
    moveAbortController = null;
    if (dataAbortController) dataAbortController.abort();
    stopRequestsInFlight += 1;
    return issueCmd(move)
      .catch(() => {})
      .then(() => {
        stopRequestsInFlight -= 1;
        flushMoveCommand();
      });
  }

  // Keep only the newest unsent direction/heartbeat and never build a queue.
  pendingMove = move;
  flushMoveCommand();
  return Promise.resolve();
}

function issueCmd(move) {
  commandSequence += 1;
  const issuedSequence = commandSequence;
  const query = '/cmd?move=' + encodeURIComponent(move) +
    '&seq=' + encodeURIComponent(issuedSequence) +
    (controlSessionToken ? '&token=' + encodeURIComponent(controlSessionToken) : '');
  const controller = new AbortController();
  if (move !== 'STOP' && move !== 'RELEASE') moveAbortController = controller;
  const started = performance.now();
  return fetch(query, {
    signal:controller.signal,
    cache:'no-store',
    keepalive:move === 'STOP' || move === 'RELEASE'
  }).then(response => {
    if (!response.ok) throw new Error('Control request rejected');
    if (issuedSequence === commandSequence) {
      lastControlRttMs = Math.max(0, Math.round(performance.now() - started));
      setText('controlRtt', lastControlRttMs + ' ms');
    }
    return response;
  }).finally(() => {
    if (move !== 'STOP' && move !== 'RELEASE' && moveAbortController === controller) {
      moveAbortController = null;
    }
  });
}

function flushMoveCommand() {
  if (moveRequestInFlight || stopRequestsInFlight > 0 || !pendingMove) return;
  const move = pendingMove;
  pendingMove = null;
  moveRequestInFlight = true;
  issueCmd(move)
    .catch(() => {})
    .then(() => {
      moveRequestInFlight = false;
      flushMoveCommand();
    });
}

function requestSensorHead(orientation) {
  if (mode !== 'MANUAL') {
    setText('sensorHeadStatus', 'Manual mode required');
    return Promise.resolve();
  }

  sensorHeadRequestedOrientation = orientation;
  sensorHeadRequestSequence = latestData ? latestData.distanceSampleSequence : 0;
  setText('sensorHeadStatus', 'Moving ' + orientation.toLowerCase() + '...');

  return fetch('/sensor-head?set=' + encodeURIComponent(orientation))
    .then(response => {
      if (response.ok) return response.text();
      return response.text().then(message => {
        throw new Error(message || 'Request rejected');
      });
    })
    .then(() => loadData())
    .catch(error => {
      sensorHeadRequestedOrientation = null;
      setText('sensorHeadStatus', error.message || 'Request rejected');
    });
}

function clearMoveRefresh() {
  if (moveRefreshTimer) clearInterval(moveRefreshTimer);
  moveRefreshTimer = null;
  activeMove = null;
}

function startMove(move) {
  if (mode !== 'MANUAL') return;
  clearMoveRefresh();
  activePress = true;
  activeMove = move;
  sendCmd(move);
  moveRefreshTimer = setInterval(() => {
    if (activePress && activeMove && mode === 'MANUAL') sendCmd(activeMove);
  }, MOVE_REFRESH_MS);
}

function stopMove() {
  if (!activePress) return;
  activePress = false;
  activePointerId = null;
  clearMoveRefresh();
  sendCmd('RELEASE');
}

function explicitStop() {
  activePress = false;
  activePointerId = null;
  clearMoveRefresh();
  sendCmd('STOP');
}

function updateModeButtons() {
  const autoButton = byId('autoBtn');
  const manualButton = byId('manualBtn');
  const dpad = byId('dpad');
  const turnButton = byId('turnModeBtn');
  const manualActive = mode === 'MANUAL';
  if (!manualActive && activePress) {
    activePress = false;
    clearMoveRefresh();
  }
  if (autoButton) autoButton.classList.toggle('active', mode === 'AUTO');
  if (manualButton) manualButton.classList.toggle('active', manualActive);
  if (dpad) dpad.classList.toggle('auto', !manualActive);
  document.querySelectorAll('.hold').forEach(button => {
    button.disabled = !manualActive;
    button.setAttribute('aria-disabled', manualActive ? 'false' : 'true');
  });
  const stopButton = byId('stopBtn');
  if (stopButton) {
    stopButton.disabled = false;
    stopButton.setAttribute('aria-disabled', 'false');
  }
  if (turnButton) turnButton.disabled = !manualActive;
  setText('controlAvailability', manualActive
    ? 'Manual controls active. Release stops motors; an independent control session maintains override permission.'
    : 'Directional controls are unavailable in AUTO mode. STOP remains available.');
}

function updateSensorHeadControls(d) {
  const manualActive = mode === 'MANUAL';
  const inspectionOrientation = d && d.manualInspectionOrientation ? d.manualInspectionOrientation : 'NONE';
  const inspectionWarning = Boolean(d && d.manualInspectionObstacle);
  const status = byId('sensorHeadStatus');
  document.querySelectorAll('.sensor-head-btn').forEach(button => {
    button.disabled = !manualActive;
    button.setAttribute('aria-disabled', manualActive ? 'false' : 'true');
    button.classList.toggle('active', Boolean(d) && d.scanOrientation === button.dataset.sensorHead);
  });

  setText('sensorHeadOrientation', d && d.scanOrientation ? d.scanOrientation : '--');
  setText('sensorHeadAvailability', manualActive ? 'Select a fixed inspection direction' : 'Available in Manual mode');
  if (status) status.classList.toggle('warning', inspectionWarning);

  if (!manualActive) {
    sensorHeadRequestedOrientation = null;
    setText('sensorHeadStatus', 'Manual mode required');
  } else if (inspectionWarning) {
    sensorHeadRequestedOrientation = null;
    setText('sensorHeadStatus', manualInspectionWarningText(d));
  } else if (sensorHeadRequestedOrientation &&
             d.navigationDecision === 'MANUAL_SENSOR_READY' &&
             d.scanOrientation === sensorHeadRequestedOrientation &&
             d.distanceSampleSequence > sensorHeadRequestSequence) {
    sensorHeadRequestedOrientation = null;
    setText('sensorHeadStatus', 'Inspecting ' + inspectionOrientation);
  } else if (!sensorHeadRequestedOrientation) {
    setText('sensorHeadStatus', inspectionOrientation !== 'NONE'
      ? 'Inspecting ' + inspectionOrientation
      : 'Sensor head ready');
  }
}

function updateTurnModeButton() {
  const button = byId('turnModeBtn');
  if (!button) return;
  button.textContent = 'Turn Mode: ' + turnMode;
  button.classList.toggle('spin', turnMode === 'SPIN');
}

function ensureAudio() {
  if (!audioCtx) {
    const AudioContextClass = window.AudioContext || window.webkitAudioContext;
    if (!AudioContextClass) {
      setAudioStatus('AUDIO NOT SUPPORTED');
      return;
    }
    audioCtx = new AudioContextClass();
    alarmOutput = audioCtx.createGain();
    alarmOutput.gain.value = 1.0;
    alarmOutput.connect(audioCtx.destination);
  }
  if (audioCtx.state === 'suspended') audioCtx.resume();
}

function setAudioStatus(text) {
  setText('audioStatus', text);
}

function beep(freq, ms, delayMs = 0, level = 0.26) {
  if (!alarmEnabled || !audioCtx || !alarmOutput) return;
  ensureAudio();
  const osc = audioCtx.createOscillator();
  const gain = audioCtx.createGain();
  const startAt = audioCtx.currentTime + Math.max(0, delayMs) / 1000;
  const stopAt = startAt + Math.max(40, ms) / 1000;
  osc.frequency.value = freq;
  osc.type = 'sine';
  gain.gain.setValueAtTime(0.0001, startAt);
  gain.gain.exponentialRampToValueAtTime(level, startAt + 0.015);
  gain.gain.setValueAtTime(level, Math.max(startAt + 0.016, stopAt - 0.035));
  gain.gain.exponentialRampToValueAtTime(0.0001, stopAt);
  osc.connect(gain);
  gain.connect(alarmOutput);
  osc.start(startAt);
  osc.stop(stopAt + 0.01);
  osc.onended = () => {
    osc.disconnect();
    gain.disconnect();
  };
}

function urgentDoubleBeep() {
  beep(980, 150, 0, 0.30);
  beep(760, 170, 240, 0.30);
}

function vibrateHighRisk() {
  if (navigator.vibrate) navigator.vibrate([120, 80, 120]);
}

function stopAlarmTimers() {
  if (highRiskTimer) {
    clearInterval(highRiskTimer);
    highRiskTimer = null;
  }
  if (highRiskVibeTimer) {
    clearInterval(highRiskVibeTimer);
    highRiskVibeTimer = null;
  }
  if (navigator.vibrate) navigator.vibrate(0);
}

function enableAlarm(event) {
  if (event) event.preventDefault();
  ensureAudio();
  alarmEnabled = true;
  alarmMuted = false;
  fetch('/alarm/enable').then(() => loadData()).catch(() => {});
  if (alarmOutput && audioCtx) {
    alarmOutput.gain.cancelScheduledValues(audioCtx.currentTime);
    alarmOutput.gain.setValueAtTime(1.0, audioCtx.currentTime);
  }
  setAudioStatus('AUDIO ENABLED');
  beep(900, 250, 0, 0.30);
}

function testBeep(event) {
  if (event) event.preventDefault();
  ensureAudio();
  fetch('/alarm/test').then(() => loadData()).catch(() => {});
  const previousEnabled = alarmEnabled;
  const previousMuted = alarmMuted;
  alarmEnabled = true;
  alarmMuted = false;
  beep(900, 250, 0, 0.30);
  alarmEnabled = previousEnabled;
  alarmMuted = previousMuted;
}

function disableAlarm(event) {
  if (event) event.preventDefault();
  alarmEnabled = false;
  alarmMuted = true;
  fetch('/alarm/mute').then(() => loadData()).catch(() => {});
  stopAlarmTimers();
  if (alarmOutput && audioCtx) {
    alarmOutput.gain.cancelScheduledValues(audioCtx.currentTime);
    alarmOutput.gain.setValueAtTime(0.0, audioCtx.currentTime);
  }
  setAudioStatus('ALARM MUTED');
}

function addReason(reasons, text) {
  if (text && !reasons.includes(text)) reasons.push(text);
}

function readableAlarmReason(reason) {
  const names = {
    FLAME:'Flame detected',
    GAS_HIGH_RELATIVE:'MQ-2 high relative reading',
    TEMP_HIGH:'Temperature critical',
    NEAR_OBSTACLE:'Near obstacle',
    IR_OBSTACLE:'IR obstacle',
    LEFT_IR_OBSTACLE:'Left IR obstacle',
    RIGHT_IR_OBSTACLE:'Right IR obstacle',
    BOTH_IR_OBSTACLES:'Both IR obstacles',
    ULTRASONIC_WARNING:'Ultrasonic warning',
    ULTRASONIC_SENSOR_FAULT:'Ultrasonic sensor fault',
    ULTRASONIC_STALE:'Ultrasonic data stale',
    GAS_ELEVATED_RELATIVE:'MQ-2 elevated relative reading',
    TEMP_WARN:'Temperature warning'
  };
  return names[reason] || (reason && reason !== 'NONE' ? reason.replace(/_/g, ' ') : '');
}

function manualInspectionWarningText(d) {
  if (!d || !d.manualInspectionObstacle) return '';
  const orientation = d.manualInspectionOrientation || 'UNKNOWN';
  const distance = d.distanceValid ? ' \u2014 ' + d.distance + ' cm' : '';
  return 'Obstacle near ' + orientation + distance;
}

function startHighRiskTimers() {
  if (highRiskTimer) return;
  urgentDoubleBeep();
  vibrateHighRisk();
  highRiskTimer = setInterval(() => urgentDoubleBeep(), 900);
  highRiskVibeTimer = setInterval(() => vibrateHighRisk(), 1200);
}

function controlPost(path, values) {
  return fetch(path, {
    method:'POST', cache:'no-store',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:new URLSearchParams(values)
  }).then(async response => {
    if (response.ok) return response;
    throw new Error(await response.text());
  });
}

function ensureControlSession() {
  if (mode !== 'MANUAL' || document.hidden || controlSessionToken || sessionRequestInFlight) return;
  sessionRequestInFlight = true;
  controlPost('/control-session', {}).then(response => response.json())
    .then(result => {
      controlSessionToken = result.token;
      setText('overrideStatus', 'Override OFF; control session active');
    }).catch(() => setText('overrideStatus', 'Control session unavailable'))
    .finally(() => { sessionRequestInFlight = false; });
}

function toggleManualOverride() {
  if (mode !== 'MANUAL' || !controlSessionToken) return;
  const enable = !(latestData && latestData.manualFlameGasOverride);
  if (enable && !window.confirm(
      'Flame/gas hazard is active. Manual movement can expose people and equipment. ' +
      'Keep direct supervision and an accessible STOP. Rear clearance is not sensor-verified. Enable override?')) return;
  controlPost('/manual-override', {
    set:enable ? 'on' : 'off', token:controlSessionToken,
    confirm:enable ? '1' : '0'
  }).then(() => {
    setText('overrideStatus', enable ? 'Override enabled' : 'Override OFF');
    loadData();
  }).catch(error => setText('overrideStatus', 'Override rejected: ' + error.message));
}

function updateAlarmBanner(d) {
  const indicator = byId('alarmIndicator');
  const reasons = [];
  const info = [];
  const gasStatus = d.gasStatus || 'DISABLED';
  const safetyState = d.safetyState || 'SAFE';
  const sensorFault = d.ultrasonicState === 'SENSOR_FAULT' || d.ultrasonicState === 'STALE';
  const manualInspectionObstacle = Boolean(d.manualInspectionObstacle);
  const manualInspectionReason = manualInspectionWarningText(d);

  if (gasStatus === 'HIGH_RELATIVE') addReason(reasons, 'MQ-2 high relative reading');
  if (gasStatus === 'ELEVATED_RELATIVE') addReason(reasons, 'MQ-2 elevated relative reading');
  if (d.flameDetected) addReason(reasons, 'Flame detected');
  if (manualInspectionObstacle) {
    addReason(reasons, manualInspectionReason);
  } else if (d.nearObstacleHazard) {
    addReason(reasons, 'Near obstacle' + (d.nearObstacleSource && d.nearObstacleSource !== 'NONE' ? ' (' + d.nearObstacleSource + ')' : ''));
  } else if (d.irObstacleDetected) {
    addReason(reasons, 'IR obstacle');
  }
  if (sensorFault) {
    addReason(reasons, 'Ultrasonic sensor fault');
  } else if (d.ultrasonicWarning) {
    addReason(reasons, 'Ultrasonic warning');
  }
  const primaryReasonAlreadyRepresented =
    (d.alarmReason === 'NEAR_OBSTACLE' && d.nearObstacleHazard) ||
    (d.alarmReason === 'ULTRASONIC_WARNING' && (sensorFault || d.ultrasonicWarning));
  if (!primaryReasonAlreadyRepresented) addReason(reasons, readableAlarmReason(d.alarmReason));
  if (d.mq2Warmup) info.push('MQ-2 warming');
  if (!d.flame_sample_valid) info.push('Flame DO sample unavailable');
  if (d.alarmSoundState === 'MUTED') info.push('Alarm sound muted');

  let severity = 'normal';
  let label = 'SYSTEM NORMAL';
  if (d.manualFlameGasOverride) {
    severity = 'critical';
    label = 'HAZARD ACTIVE ? MANUAL OVERRIDE';
  } else if (safetyState === 'CRITICAL') {
    severity = 'critical';
    label = 'CRITICAL';
  } else if (d.nearObstacleHazard) {
    severity = 'near';
    label = 'NEAR OBSTACLE';
  } else if (manualInspectionObstacle) {
    severity = 'near';
    label = 'INSPECTION OBSTACLE';
  } else if (sensorFault || d.ultrasonicWarning) {
    severity = 'fault';
    label = sensorFault ? 'SENSOR FAULT' : 'ULTRASONIC WARNING';
  } else if (gasStatus === 'HIGH_RELATIVE') {
    severity = 'high';
    label = 'GAS WARNING';
  } else if (d.flameDetected) {
    severity = 'high';
    label = 'FLAME DETECTED';
  } else if (d.irObstacleDetected) {
    severity = 'near';
    label = 'IR OBSTACLE';
  } else if (safetyState === 'CAUTION' || gasStatus === 'ELEVATED_RELATIVE') {
    severity = 'caution';
    label = 'CAUTION';
  }

  if (indicator) {
    indicator.className = 'alarm-indicator ' + severity;
    const labelElement = indicator.querySelector('.alarm-label');
    const detailElement = indicator.querySelector('.alarm-detail');
    if (labelElement) labelElement.textContent = label;
    const detailParts = reasons.length ? reasons : ['No active warning'];
    info.forEach(item => detailParts.push(item));
    if (detailElement) detailElement.textContent = detailParts.join(' | ');
    indicator.setAttribute('role', severity === 'normal' ? 'status' : 'alert');
    indicator.setAttribute('aria-live', severity === 'normal' ? 'polite' : 'assertive');
  }

  const normal = safetyState === 'SAFE' &&
    (gasStatus === 'LOW_RELATIVE' || gasStatus === 'WARMING' || gasStatus === 'DISABLED') &&
    !d.nearObstacleHazard &&
    !manualInspectionObstacle;
  if (normal) {
    stopAlarmTimers();
    return;
  }
  if (!alarmEnabled || alarmMuted) return;
  ensureAudio();
  if (safetyState === 'CRITICAL') {
    startHighRiskTimers();
  } else if (d.nearObstacleHazard) {
    startHighRiskTimers();
  } else if (manualInspectionObstacle) {
    startHighRiskTimers();
  } else if (safetyState === 'CAUTION' || gasStatus === 'ELEVATED_RELATIVE') {
    stopAlarmTimers();
    if (Date.now() - lastCautionBeep > 5000) {
      lastCautionBeep = Date.now();
      beep(560, 160, 0, 0.25);
    }
  }
}

function angleToCanvas(angleDeg, distanceCm, cx, cy, radius) {
  const clampedDistance = Math.max(0, Math.min(RADAR_MAX_CM, distanceCm));
  const r = radius * (clampedDistance / RADAR_MAX_CM);
  const theta = (180 - Math.max(0, Math.min(180, angleDeg))) * Math.PI / 180;
  return { x:cx + Math.cos(theta) * r, y:cy - Math.sin(theta) * r };
}

function rememberRadarSample(d) {
  if (!d || !d.distanceValid || d.distanceSampleSequence === lastDistanceSampleSequence) return;
  lastDistanceSampleSequence = d.distanceSampleSequence;
  const bucket = Math.round((d.servoAngle || 90) / 5) * 5;
  const now = Date.now();
  const existing = scanSamples.findIndex(sample => sample.bucket === bucket);
  const next = { bucket, angle:d.servoAngle || bucket, distance:d.distance, t:now };
  if (existing >= 0) {
    scanSamples[existing] = next;
  } else {
    scanSamples.push(next);
  }
  scanSamples.sort((a, b) => a.t - b.t);
  while (scanSamples.length > 10) scanSamples.shift();
}

function drawRadar(d) {
  if (!d) return;
  const canvas = byId('radar');
  if (!canvas || canvas.offsetParent === null) return;
  const ctx = canvas.getContext('2d');
  if (!ctx) return;
  const w = canvas.width;
  const h = canvas.height;
  const cx = w / 2;
  const cy = h - 18;
  const radius = Math.min(w / 2 - 18, h - 30);
  ctx.clearRect(0, 0, w, h);
  ctx.strokeStyle = d.nearObstacleHazard ? '#ef4444' : '#155e75';
  ctx.lineWidth = d.nearObstacleHazard ? 3 : 1;
  RADAR_RINGS_CM.forEach(ring => {
    ctx.beginPath();
    ctx.arc(cx, cy, radius * ring / RADAR_MAX_CM, Math.PI, 0);
    ctx.stroke();
    ctx.fillStyle = '#67e8f9';
    ctx.font = '10px Arial';
    ctx.fillText(ring + 'cm', cx + radius * ring / RADAR_MAX_CM - 22, cy - 3);
  });
  for (let angle = 0; angle <= 180; angle += 45) {
    const point = angleToCanvas(angle, RADAR_MAX_CM, cx, cy, radius);
    ctx.beginPath();
    ctx.moveTo(cx, cy);
    ctx.lineTo(point.x, point.y);
    ctx.stroke();
  }
  const now = Date.now();
  for (let index = scanSamples.length - 1; index >= 0; index--) {
    if (now - scanSamples[index].t > 1800) scanSamples.splice(index, 1);
  }
  scanSamples.forEach(sample => {
    const point = angleToCanvas(sample.angle, sample.distance, cx, cy, radius);
    const alpha = Math.max(0.25, 1 - (now - sample.t) / 1800);
    ctx.fillStyle = sample.distance <= 7 ? 'rgba(239,68,68,' + alpha + ')' : 'rgba(250,204,21,' + alpha + ')';
    ctx.beginPath();
    ctx.arc(point.x, point.y, sample.distance <= 7 ? 6 : 4, 0, Math.PI * 2);
    ctx.fill();
  });
  const sweep = angleToCanvas(d.servoAngle || 90, RADAR_MAX_CM, cx, cy, radius);
  ctx.strokeStyle = d.nearObstacleHazard ? '#f87171' : '#22d3ee';
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(cx, cy);
  ctx.lineTo(sweep.x, sweep.y);
  ctx.stroke();
  ctx.fillStyle = '#e5e7eb';
  ctx.font = '12px Arial';
  ctx.fillText('L', 10, cy - 6);
  ctx.fillText('F', cx - 4, 14);
  ctx.fillText('R', w - 18, cy - 6);
  setText('radarReadout',
    'Servo-scanned ultrasonic: ' + d.scanOrientation + ' @ ' + d.servoAngle + ' deg, ' +
    (d.distanceValid ? d.distance + ' cm' : d.ultrasonicState) +
    (d.nearObstacleHazard ? ' - NEAR OBSTACLE' : ''));
}

function radarLoop(timestamp) {
  const controlSection = byId('control');
  if (!document.hidden && controlSection && !controlSection.hidden && timestamp - lastRadarDraw >= 66) {
    lastRadarDraw = timestamp;
    drawRadar(latestData);
  }
  requestAnimationFrame(radarLoop);
}

function updateWiFi(d) {
  const softApIp = d.softApIp || '192.168.4.1';
  const staUrl = d.staConnected ? 'http://' + d.staIp : '--';
  const mdnsHost = d.localHostname || 'piku.local';
  setText('apStatus', 'ALWAYS AVAILABLE');
  setText('apIp', softApIp);
  setText('apChannel', d.softApChannel ?? '--');
  setText('apClients', d.softApClients ?? 0);
  setText('localHostname', mdnsHost);
  setText('apUrl', 'http://' + softApIp);
  setText('staConfiguredStatus', d.staConfigured ? 'YES' : 'NO');
  setText('savedSsid', d.staSsid || '--');
  setText('staStatus', d.staStatus || 'NOT_CONFIGURED');
  setText('staIp', d.staConnected ? d.staIp : '--');
  setText('staUrl', staUrl);
  setText('mdnsUrl', d.staConnected ? 'http://' + mdnsHost : '--');
  setText('mdnsStatus', d.mdnsActive ? 'ACTIVE' : 'INACTIVE');
  setText('staRssi', d.staConnected ? d.staRssi + ' dBm' : '--');
  const gauge = byId('wifiGaugeFill');
  if (gauge) gauge.style.width = (d.staConnected ? d.staSignalQuality : 0) + '%';
  setText('wifiProvisionStatus',
    'SoftAP remains available at ' + softApIp + '. Home Wi-Fi: ' +
    (d.staStatus || 'NOT_CONFIGURED') +
    (d.staConnected ? ' / ' + d.staIp : '') +
    '. Your phone or laptop must use the same network to open the STA address.');

  const wifiState = d.staConnected ? 'CONNECTED' : 'SOFTAP';
  const wifiSummary = d.staConnected
    ? (d.staSsid || 'Home Wi-Fi') + ' / ' + d.staIp
    : 'PIKU SoftAP / ' + softApIp;
  setBound('wifiState', wifiState);
  setBound('wifiSummary', wifiSummary);
  setBound('softApIp', softApIp);
  const provisioningAllowed = d.mode === 'MANUAL' && d.motorState === 'STOP';
  document.querySelectorAll('.wifi-provision-action').forEach(button => {
    button.disabled = !provisioningAllowed;
    button.setAttribute('aria-disabled', provisioningAllowed ? 'false' : 'true');
  });
  if (!provisioningAllowed) {
    setWifiProvisionStatus('Wi-Fi actions require MANUAL mode with the robot STOPPED. SoftAP remains available.');
  }
}

function setWifiProvisionStatus(text) {
  setText('wifiProvisionStatus', text);
}

function scheduleWifiScanRetry() {
  if (wifiScanTimer) clearTimeout(wifiScanTimer);
  wifiScanTimer = setTimeout(() => {
    wifiScanTimer = null;
    scanWifi();
  }, 800);
}

function scanWifi(event) {
  if (event) {
    event.preventDefault();
    if (wifiScanTimer) {
      clearTimeout(wifiScanTimer);
      wifiScanTimer = null;
    }
  }
  if (!latestData || latestData.mode !== 'MANUAL' || latestData.motorState !== 'STOP') {
    setWifiProvisionStatus('Stop the robot and select MANUAL before scanning');
    return;
  }
  if (wifiScanInFlight) return;
  wifiScanInFlight = true;
  setWifiProvisionStatus('Scanning...');
  fetch('/wifi/scan')
    .then(response => {
      if (!response.ok) throw new Error('Wi-Fi scan failed');
      return response.json();
    })
    .then(showWifiScan)
    .catch(() => setWifiProvisionStatus('Scan failed'))
    .then(() => {
      wifiScanInFlight = false;
    });
}

function showWifiScan(result) {
  if (result.running) {
    setWifiProvisionStatus('Scanning...');
    scheduleWifiScanRetry();
    return;
  }
  if (wifiScanTimer) {
    clearTimeout(wifiScanTimer);
    wifiScanTimer = null;
  }
  if (result.blocked) {
    setWifiProvisionStatus('Scan unavailable: ' + (result.reason || 'blocked'));
    return;
  }
  const select = byId('wifiNetworks');
  if (!select) return;
  while (select.firstChild) select.removeChild(select.firstChild);
  const placeholder = document.createElement('option');
  placeholder.value = '';
  placeholder.textContent = 'Select network';
  select.appendChild(placeholder);
  (result.networks || []).forEach(network => {
    const option = document.createElement('option');
    option.value = network.ssid;
    option.dataset.secure = network.secure ? 'true' : 'false';
    option.textContent = network.ssid + ' (' + network.rssi + ' dBm, ' +
      (network.security || (network.secure ? 'SECURED' : 'OPEN')) + ')';
    select.appendChild(option);
  });
  setWifiProvisionStatus((result.networks || []).length + ' networks found');
}

function selectWifiNetwork() {
  const select = byId('wifiNetworks');
  const input = byId('wifiSsid');
  if (select && input && select.value) input.value = select.value;
}

function connectWifi(event) {
  if (event) event.preventDefault();
  if (!latestData || latestData.mode !== 'MANUAL' || latestData.motorState !== 'STOP') {
    setWifiProvisionStatus('Stop the robot and select MANUAL before connecting');
    return;
  }
  const ssidInput = byId('wifiSsid');
  const passwordInput = byId('wifiPassword');
  const select = byId('wifiNetworks');
  const ssid = ssidInput ? ssidInput.value.trim() : '';
  const password = passwordInput ? passwordInput.value : '';
  const selected = select && select.selectedOptions.length ? select.selectedOptions[0] : null;
  if (!ssid) {
    setWifiProvisionStatus('Enter an SSID');
    return;
  }
  if (selected && selected.value === ssid && selected.dataset.secure === 'true' && !password) {
    setWifiProvisionStatus('Selected network is secured; enter its Wi-Fi password');
    return;
  }
  const body = new URLSearchParams();
  body.set('ssid', ssid);
  body.set('password', password);
  fetch('/wifi/connect', {
    method:'POST',
    headers:{ 'Content-Type':'application/x-www-form-urlencoded' },
    body
  })
    .then(response => {
      if (!response.ok) return response.json().then(body => { throw new Error(body.error || 'Connect rejected'); });
      if (passwordInput) passwordInput.value = '';
      setWifiProvisionStatus('Connecting; credentials save only after association succeeds');
      loadData();
    })
    .catch(error => setWifiProvisionStatus(error.message || 'Connect request failed'));
}

function forgetWifi(event) {
  if (event) event.preventDefault();
  if (!latestData || latestData.mode !== 'MANUAL' || latestData.motorState !== 'STOP') {
    setWifiProvisionStatus('Stop the robot and select MANUAL before forgetting Wi-Fi');
    return;
  }
  const passwordInput = byId('wifiPassword');
  fetch('/wifi/forget', { method:'POST' })
    .then(response => {
      if (!response.ok) return response.json().then(body => { throw new Error(body.error || 'Forget rejected'); });
      if (passwordInput) passwordInput.value = '';
      loadData();
    })
    .catch(error => setWifiProvisionStatus(error.message || 'Forget request failed'));
}

function reconnectWifi(event) {
  if (event) event.preventDefault();
  if (!latestData || latestData.mode !== 'MANUAL' || latestData.motorState !== 'STOP') {
    setWifiProvisionStatus('Stop the robot and select MANUAL before reconnecting');
    return;
  }
  fetch('/wifi/reconnect', { method:'POST', cache:'no-store' })
    .then(response => {
      if (!response.ok) return response.json().then(body => { throw new Error(body.error || 'Reconnect rejected'); });
      setWifiProvisionStatus('Reconnecting; SoftAP remains available');
      return loadData();
    })
    .catch(error => setWifiProvisionStatus(error.message || 'Reconnect request failed'));
}

function renderTelemetry(d) {
  latestData = d;
  const receivedCommandSequence = Number(d.commandSequence);
  if (Number.isFinite(receivedCommandSequence)) {
    commandSequence = Math.max(commandSequence, receivedCommandSequence);
  }
  rememberRadarSample(d);

  const temperatureText = d.temperature === null ? '-- C' : d.temperature + ' C';
  const humidityText = d.humidity === null ? '-- %' : d.humidity + ' %';
  const distanceText = d.distanceValid ? d.distance + ' cm' : '-- cm';
  const flameText = d.flame_sample_valid
    ? (d.flameDetected ? 'FLAME_DETECTED' : 'CLEAR')
    : 'INVALID';
  const imu = d.imu || {};
  const radio = d.radio || {};
  const display = d.display || {};
  const autoNav = d.autoNavigation || {};
  const autoScan = autoNav.scan || {};
  const metric = (value, digits, suffix) => value === null || value === undefined || !Number.isFinite(Number(value))
    ? '--' : Number(value).toFixed(digits) + (suffix || '');
  const ageText = value => value === null || value === undefined ? '--' : String(value) + ' ms';
  const irText = d.irObstacleDetected ? 'DETECTED' : 'CLEAR';
  const leftIrText = d.left_obstacle ? 'DETECTED' : 'CLEAR';
  const rightIrText = d.right_obstacle ? 'DETECTED' : 'CLEAR';
  const uptimeSeconds = Math.floor((Number(d.uptimeMs) || 0) / 1000);
  const uptimeText = Math.floor(uptimeSeconds / 3600) + 'h ' + Math.floor((uptimeSeconds % 3600) / 60) + 'm ' + (uptimeSeconds % 60) + 's';
  const firmwareSpeed = Math.max(0, Math.min(100, Number(d.speed_percent) || 0));
  if (requestedSpeedPercent === null) {
    const speedSlider = byId('motorSpeed');
    if (speedSlider) speedSlider.value = firmwareSpeed;
    setText('speedPercent', firmwareSpeed + '%');
  }
  const nearText = d.nearObstacleHazard ? 'ACTIVE' : 'CLEAR';
  const manualInspectionReason = manualInspectionWarningText(d);
  const reasonText = manualInspectionReason ||
    (d.alarmReason && d.alarmReason !== 'NONE' ? d.alarmReason : 'No active alarm');

  setText('temp', temperatureText);
  setText('hum', humidityText);
  setText('gas', d.gasValue);
  setText('gasFiltered', d.gasFilteredValue);
  setText('mq2Warmup', d.mq2Warmup ? 'WARMING' : 'READY');
  setText('gasStatus', d.gasStatus);
  setText('flame', flameText);
  setText('envTemp', d.temperature_valid ? metric(d.temperature, 1, ' C') : 'INVALID');
  setText('envHum', d.humidity_valid ? metric(d.humidity, 1, ' %') : 'INVALID');
  setText('envGasRaw', d.gasValue ?? '--');
  setText('envGasFiltered', d.gasFilteredValue ?? '--');
  setText('envGasState', d.mq2Warmup ? 'WARMING' : (d.gasStatus || '--'));
  setText('envFlame', flameText);
  setText('envDhtDiag', 'age ' + ageText(d.environment_sample_age_ms) + ' / errors ' + (d.dht_error_count ?? 0));
  setText('envMq2Diag', 'sample ' + ageText(d.mq2_sample_age_ms) + ' / warm-up ' + ageText(d.mq2_warmup_age_ms));
  setText('envFlameDiag', 'input DIGITAL DO / level ' + (d.flame_digital_level === 0 ? 'LOW' : 'HIGH') + ' / active level ' + (d.flame_active_level || 'HIGH') + ' / raw active ' + (d.flame_raw_active ? 'YES' : 'NO') + ' / debounced ' + flameText + ' / detect/clear persistence ' + ageText(d.flame_detect_persist_ms) + '/' + ageText(d.flame_clear_persist_ms) + ' / logic sample ' + (d.flame_sample_valid ? 'VALID' : 'INVALID') + ' (not a connectivity self-test) / age ' + ageText(d.flame_sample_age_ms));
  setText('imuStatus', imu.health_state || (imu.initialized && imu.healthy ? 'HEALTHY' : 'ERROR'));
  setText('imuRoll', metric(imu.roll, 1, ' deg'));
  setText('imuPitch', metric(imu.pitch, 1, ' deg'));
  setText('imuAccel', [imu.accel_x, imu.accel_y, imu.accel_z].map(v => metric(v, 2, '')).join(' / ') + ' m/s2');
  setText('imuGyro', [imu.gyro_x, imu.gyro_y, imu.gyro_z].map(v => metric(v, 2, '')).join(' / ') + ' rad/s');
  setText('imuMotion', imu.motion || 'ERROR');
  setText('imuAge', ageText(imu.sample_age_ms));
  setText('imuDiag', (imu.health_state || 'UNKNOWN') + ' / errors ' + (imu.error_count ?? 0) + ' / consecutive ' + (imu.consecutive_failures ?? 0));
  setText('radioHardware', radio.hardware || 'ERROR');
  setText('radioPeer', radio.peer || 'UNSEEN');
  setText('radioListening', radio.listening ? 'YES' : 'NO');
  setText('radioConfig', (radio.channel ?? '--') + ' / ' + (radio.data_rate || '--'));
  setText('radioPa', radio.pa_level || '--');
  setText('radioTx', (radio.tx_attempts ?? 0) + ' / ' + (radio.tx_success ?? 0) + ' / ' + (radio.tx_failure ?? 0));
  setText('radioRx', radio.rx_packets ?? 0);
  setText('radioTxAge', ageText(radio.last_tx_age_ms));
  setText('radioRxAge', ageText(radio.last_rx_age_ms));
  setText('displayState', display.initialized ? 'READY' : 'ERROR');
  setText('commRadioEnabled', d.radioEnabled ? 'ENABLED' : 'DISABLED');
  setText('commRadioHardware', d.radioEnabled ? (radio.hardware || 'ERROR') : 'DISABLED');
  setText('commRadioPeer', d.radioEnabled ? (radio.peer || 'UNSEEN') : 'DISABLED');
  setText('commRadioHeartbeat', ageText(radio.last_tx_age_ms));
  setText('commRadioRxAge', ageText(radio.last_rx_age_ms));
  setText('commRadioTx', (radio.tx_success ?? 0) + ' / ' + (radio.tx_failure ?? 0));
  setText('commRadioRx', radio.rx_packets ?? 0);
  setText('irObstacle', irText);
  setText('leftIrObstacle', leftIrText);
  setText('rightIrObstacle', rightIrText);
  setText('leftIrRaw', d.left_ir_raw);
  setText('rightIrRaw', d.right_ir_raw);
  setText('sensorDistance', distanceText);
  setText('sensorServo', d.scanOrientation + ' / ' + d.servoAngle + ' deg');
  setText('nearObstacle', nearText);
  setText('nearSource', d.nearObstacleSource);
  setText('ultraWarn', d.ultrasonicWarning ? 'WARNING' : 'CLEAR');
  setText('ultraState', d.ultrasonicState || '--');
  setText('ultraDiag', 'age ' + ageText(d.distanceSampleAgeMs) + ' / seq ' + (d.distanceSampleSequence ?? 0) + ' / invalid ' + (d.ultrasonicInvalidCount ?? 0) + ' / faults ' + (d.ultrasonicFaultCount ?? 0));
  setText('safety', d.safetyState);
  setText('alarmReason', reasonText);
  setText('alarmSound', d.alarmSoundState);
  setText('buzzer', d.buzzerState);
  setText('dist', distanceText);
  setText('scan', d.scanOrientation + ' / ' + d.servoAngle + ' deg');
  setText('motor', d.motorState);
  setText('nav', d.navigationDecision);
  setText('mode', d.mode);
  setText('turnMode', d.turnMode);
  setText('controlCommandAge', ageText(d.commandAgeMs));
  setText('controlRtt', lastControlRttMs === null ? '--' : lastControlRttMs + ' ms');
  const scanText = key => {
    const sample = autoScan[key] || {};
    if (sample.state === 'CLEAR_NO_ECHO') return 'CLEAR_NO_ECHO';
    if (sample.trusted && Number.isFinite(Number(sample.distance_cm))) return Number(sample.distance_cm).toFixed(1) + ' cm';
    return sample.state || 'NOT_SAMPLED';
  };
  setText('autoNavState', autoNav.state || '--');
  setText('autoIrSource', (autoNav.ir_zone || 'CLEAR') + ' / ' +
    (autoNav.avoidance_source || 'NONE'));
  setText('autoTurnAngle', metric(autoNav.relative_turn_deg, 1, ' deg') + ' / ' + metric(autoNav.turn_target_deg, 1, ' deg'));
  setText('autoSelectedPath', (autoNav.preferred_path || 'NONE') + ' / ' +
    (autoNav.selected_path || 'NONE'));
  setText('autoTurnResult', autoNav.turn_completion || 'NONE');
  setText('autoBlockedRetry', 'attempt ' + (autoNav.recovery_attempts ?? 0) +
    ' / blocked ' + (autoNav.blocked_retry_count ?? 0));
  setText('autoScanSummary', scanText('left') + ' / ' + scanText('front') + ' / ' + scanText('right'));
  const hazard = autoNav.hazard || {};
  setText('autoHazardRecovery', (hazard.state || 'IDLE') + (hazard.clear_remaining_ms ? ' / ' + hazard.clear_remaining_ms + ' ms remaining' : ''));
  const stuck = autoNav.stuck || {};
  setText('autoStuckRecovery', (stuck.state || 'MONITORING') + ' / attempt ' + (stuck.attempts ?? 0));

  setText('systemFirmware', d.firmware || '--');
  setText('systemUptime', uptimeText);
  setText('systemFreeHeap', (d.freeHeap ?? 0) + ' bytes');
  setText('systemMinHeap', (d.minimumFreeHeap ?? 0) + ' bytes');
  setText('systemPsram', (d.psramSize ?? 0) > 0 ? d.psramSize + ' bytes' : 'NOT DETECTED');
  setText('systemResetReason', d.resetReason || '--');
  setText('systemLoopGap', ageText(d.maximumServiceLoopGapMs));
  setText('systemHttpErrors', d.httpErrorCount ?? 0);
  setText('systemHttpTimeouts', d.httpTimeoutCount ?? 0);
  setText('systemWifiReconnects', d.wifiReconnectCount ?? 0);
  setText('systemLastAccepted', d.lastAcceptedCommand || '--');
  setText('systemLastApplied', d.lastAppliedCommand || '--');
  setText('systemCommandSequence', d.commandSequence ?? 0);
  setText('systemStopReason', d.lastStopReason || '--');
  setText('systemCommandAge', ageText(d.commandAgeMs));
  setText('systemApplyLag', ageText(d.handlerApplyLagMs));
  setText('systemDhtErrors', d.dht_error_count ?? 0);
  setText('systemUltrasonicFaults', d.ultrasonicFaultCount ?? 0);
  setText('systemImuErrors', imu.error_count ?? 0);
  setText('systemRadioFailures', radio.tx_failure ?? 0);

  setBound('temperature', temperatureText);
  setBound('humidity', humidityText);
  setBound('distance', distanceText);
  setBound('distanceValidity', d.distanceValid ? 'VALID' : 'NOT VALID');
  setBound('servoAngle', d.servoAngle + ' deg');
  setBound('scanOrientation', d.scanOrientation || '--');
  setBound('ultrasonicState', d.ultrasonicState || '--');
  setBound('flameState', flameText);
  setBound('irState', irText);
  setBound('leftIrState', leftIrText);
  setBound('rightIrState', rightIrText);
  setBound('leftIrRaw', d.left_ir_raw);
  setBound('rightIrRaw', d.right_ir_raw);
  setBound('nearObstacleState', nearText);
  setBound('nearObstacleDetail', d.nearObstacleHazard ? 'ACTIVE / ' + d.nearObstacleSource : 'CLEAR');
  setBound('gasStatus', d.gasStatus || '--');
  setBound('safetyState', d.safetyState || '--');
  setBound('alarmReason', reasonText);
  setBound('alarmSoundState', d.alarmSoundState || '--');
  setBound('motorState', d.motorState || '--');
  setBound('navigationDecision', d.navigationDecision || '--');
  setBound('mode', d.mode || '--');
  setBound('turnMode', d.turnMode || '--');
  setBound('uptime', uptimeText);
  setBound('firmware', d.firmware || '--');
  setBound('stage2Status', d.stage2Status || 'DISABLED');
  const environmentHazard = d.flame_sample_valid && d.flameDetected
    ? d.flame_state
    : (d.mq2Warmup ? 'MQ-2 WARMING' : (d.gasStatus || 'CLEAR'));
  const sensorFaults = [];
  if (d.ultrasonicState === 'SENSOR_FAULT') sensorFaults.push('ultrasonic');
  if (!d.temperature_valid || !d.humidity_valid) sensorFaults.push('DHT22');
  if (!imu.healthy) sensorFaults.push('IMU');
  if (!d.flame_sample_valid) sensorFaults.push('flame DO');
  setBound('environmentHazard', environmentHazard);
  setBound('radioSummary', d.radioEnabled ? (radio.hardware || 'ERROR') + ' / ' + (radio.peer || 'UNSEEN') : 'DISABLED');
  setBound('sensorHealth', sensorFaults.length ? 'ATTENTION' : 'HEALTHY');
  setBound('sensorHealthDetail', sensorFaults.length ? sensorFaults.join(', ') : 'All reporting valid');

  if (d.alarmSoundState === 'MUTED') {
    alarmEnabled = false;
    alarmMuted = true;
    stopAlarmTimers();
    setAudioStatus('ALARM MUTED');
  }

  mode = d.mode;
  turnMode = d.turnMode;
  if (mode === 'MANUAL' && !d.controlSessionAlive) {
    controlSessionToken = null;
    if (activePress) {
      activePress = false;
      clearMoveRefresh();
      sendCmd('STOP');
    }
  }
  if (mode === 'MANUAL') ensureControlSession();
  const overrideButton = byId('overrideBtn');
  if (overrideButton) {
    overrideButton.disabled = mode !== 'MANUAL' || !controlSessionToken;
    overrideButton.textContent = d.manualFlameGasOverride ? 'Disable manual override' : 'Enable manual flame/gas override';
  }
  const overrideAlert = byId('overrideAlert');
  if (overrideAlert) overrideAlert.hidden = !d.manualFlameGasOverride;
  if (d.manualFlameGasOverride) setText('overrideStatus', 'Override ON ? ' + (d.manualMotionInhibitReason || 'NONE'));

  updateModeButtons();
  updateSensorHeadControls(d);
  updateTurnModeButton();
  updateAlarmBanner(d);
  updateWiFi(d);
}

function updateConnectionState(state) {
  const indicator = byId('connectionState');
  if (!indicator) return;
  indicator.className = 'status-badge connection-badge is-' + state;
  const label = indicator.querySelector('strong');
  if (label) {
    label.textContent = state === 'live' ? 'Live' :
      (state === 'stale' ? 'Stale' : (state === 'offline' ? 'Offline' : 'Updating'));
  }
}

function loadData() {
  if (document.hidden) return Promise.resolve();
  if (dataRequestInFlight) return dataRequestPromise || Promise.resolve();
  if (telemetryTimer) {
    clearTimeout(telemetryTimer);
    telemetryTimer = null;
  }
  dataRequestInFlight = true;
  if (!lastTelemetrySuccess) updateConnectionState('updating');
  const controller = new AbortController();
  dataAbortController = controller;
  let timedOut = false;
  const timeout = setTimeout(() => {
    timedOut = true;
    controller.abort();
  }, TELEMETRY_TIMEOUT_MS);
  const dataUrl = controlSessionToken
    ? '/data?token=' + encodeURIComponent(controlSessionToken) : '/data';
  dataRequestPromise = fetch(dataUrl, { signal:controller.signal, cache:'no-store' })
    .then(response => {
      if (!response.ok) throw new Error('Telemetry request failed');
      return response.json();
    })
    .then(data => {
      lastTelemetrySuccess = Date.now();
      updateConnectionState(Number(data.telemetry_age_ms) > 1200 ? 'stale' : 'live');
      renderTelemetry(data);
    })
    .catch(() => {
      if (timedOut) {
        telemetryTimeoutCount += 1;
        setText('systemTelemetryTimeouts', telemetryTimeoutCount);
      }
      const age = lastTelemetrySuccess ? Date.now() - lastTelemetrySuccess : Number.POSITIVE_INFINITY;
      updateConnectionState(age > 5000 ? 'offline' : 'stale');
    })
    .then(() => {
      clearTimeout(timeout);
      if (dataAbortController === controller) dataAbortController = null;
      dataRequestInFlight = false;
      dataRequestPromise = null;
      if (!document.hidden) {
        telemetryTimer = setTimeout(() => {
          telemetryTimer = null;
          loadData();
        }, TELEMETRY_INTERVAL_MS);
      }
    });
  return dataRequestPromise;
}

function normalizedSectionName(value) {
  return ['overview', 'control', 'sensors', 'communication', 'system'].includes(value) ? value : 'overview';
}

function activateSection(sectionName, updateHash) {
  const selected = normalizedSectionName(sectionName);
  document.querySelectorAll('.dashboard-section').forEach(section => {
    const active = section.id === selected;
    section.hidden = !active;
    section.classList.toggle('active', active);
  });
  document.querySelectorAll('.nav-btn').forEach(button => {
    const active = button.dataset.section === selected;
    button.classList.toggle('active', active);
    button.setAttribute('aria-selected', active ? 'true' : 'false');
    button.tabIndex = active ? 0 : -1;
  });
  if (updateHash && window.location.hash !== '#' + selected) {
    window.location.hash = selected;
  }
}

function initializeNavigation() {
  document.querySelectorAll('[data-section]').forEach(button => {
    button.addEventListener('click', () => activateSection(button.dataset.section, true));
  });
  window.addEventListener('hashchange', () => {
    activateSection(window.location.hash.substring(1), false);
  });
  activateSection(window.location.hash.substring(1), false);
}

function initializeControls() {
  document.querySelectorAll('.hold').forEach(button => {
    button.addEventListener('pointerdown', event => {
      event.preventDefault();
      if (activePress) explicitStop();
      activePointerId = event.pointerId;
      try { button.setPointerCapture(event.pointerId); } catch (_) {}
      startMove(button.dataset.move);
    });
    button.addEventListener('pointerup', event => {
      event.preventDefault();
      if (activePointerId !== null && event.pointerId !== activePointerId) return;
      try { button.releasePointerCapture(event.pointerId); } catch (_) {}
      stopMove();
    });
    button.addEventListener('pointercancel', event => {
      if (activePointerId === null || event.pointerId === activePointerId) stopMove();
    });
    button.addEventListener('lostpointercapture', event => {
      if (activePointerId !== null && event.pointerId === activePointerId) stopMove();
    });
  });
  const stopButton = byId('stopBtn');
  if (stopButton) {
    stopButton.addEventListener('pointerdown', event => {
      event.preventDefault();
      explicitStop();
    });
  }
  window.addEventListener('pagehide', explicitStop);
  window.addEventListener('beforeunload', explicitStop);
  document.addEventListener('visibilitychange', () => {
    if (document.hidden) {
      if (telemetryTimer) clearTimeout(telemetryTimer);
      telemetryTimer = null;
      if (dataAbortController) dataAbortController.abort();
      explicitStop();
    } else {
      loadData();
    }
  });
}

function initializeDashboard() {
  if (dashboardInitialized) return;
  dashboardInitialized = true;
  initializeNavigation();
  initializeControls();
  const overrideButton = byId('overrideBtn');
  if (overrideButton) overrideButton.addEventListener('click', toggleManualOverride);
  if (!radarFrameStarted) {
    radarFrameStarted = true;
    requestAnimationFrame(radarLoop);
  }
  loadData();
}

initializeDashboard();
)js";
