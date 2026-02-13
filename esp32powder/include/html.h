#ifndef HTML_H
#define HTML_H

const char* HTML_PAGE = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>RFID Powder Tracking</title>
  <style>
    * { 
      margin: 0; 
      padding: 0; 
      box-sizing: border-box; 
    }
    
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      padding: 20px;
      color: #333;
    }
    
    .container {
      max-width: 900px;
      margin: 0 auto;
    }
    
    .header {
      text-align: center;
      color: white;
      margin-bottom: 30px;
    }
    
    .header h1 {
      font-size: 2.5em;
      margin-bottom: 10px;
      text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
    }
    
    .header p {
      font-size: 1.1em;
      opacity: 0.9;
    }
    
    .card {
      background: white;
      border-radius: 20px;
      padding: 30px;
      margin-bottom: 20px;
      box-shadow: 0 10px 40px rgba(0,0,0,0.2);
    }
    
    .status-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      margin-bottom: 20px;
    }
    
    .status-left {
      display: flex;
      align-items: center;
    }
    
    .status-indicator {
      display: inline-block;
      width: 14px;
      height: 14px;
      border-radius: 50%;
      margin-right: 10px;
    }
    
    .status-indicator.active {
      background: #4caf50;
      box-shadow: 0 0 15px #4caf50;
      animation: pulse 2s infinite;
    }
    
    .status-indicator.inactive {
      background: #ccc;
    }
    
    @keyframes pulse {
      0%, 100% { opacity: 1; transform: scale(1); }
      50% { opacity: 0.7; transform: scale(1.1); }
    }
    
    .status-text {
      font-size: 1.3em;
      font-weight: 600;
      color: #555;
    }
    
    .history-badge {
      background: linear-gradient(135deg, #84fab0 0%, #8fd3f4 100%);
      padding: 8px 15px;
      border-radius: 20px;
      font-size: 0.85em;
      font-weight: 600;
      color: #333;
    }
    
    .controls {
      display: grid;
      grid-template-columns: 1fr 1fr 1fr 1fr 1fr;
      gap: 12px;
      margin: 25px 0;
    }
    
    .btn {
      padding: 18px 15px;
      border: none;
      border-radius: 12px;
      font-size: 14px;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.3s ease;
      box-shadow: 0 4px 15px rgba(0,0,0,0.1);
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 6px;
    }
    
    .btn:hover {
      transform: translateY(-2px);
      box-shadow: 0 6px 20px rgba(0,0,0,0.2);
    }
    
    .btn:active {
      transform: translateY(0);
    }
    
    .btn-start {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
    }
    
    .btn-stop {
      background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
      color: white;
    }
    
    .btn-clear {
      background: linear-gradient(135deg, #ffecd2 0%, #fcb69f 100%);
      color: #333;
    }
    
    .btn-register {
      background: linear-gradient(135deg, #a8edea 0%, #fed6e3 100%);
      color: #333;
    }
    
    .btn-export {
      background: linear-gradient(135deg, #84fab0 0%, #8fd3f4 100%);
      color: #333;
    }
    
    .section {
      margin: 30px 0;
      padding-top: 20px;
      border-top: 2px solid #f0f0f0;
    }
    
    .section h3 {
      margin-bottom: 15px;
      color: #555;
      font-size: 1.1em;
    }
    
    .power-buttons {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 12px;
    }
    
    .btn-power {
      padding: 15px;
      font-size: 15px;
      border: none;
      border-radius: 12px;
      cursor: pointer;
      font-weight: 600;
      transition: all 0.3s;
    }
    
    .btn-power-low {
      background: linear-gradient(135deg, #e0c3fc 0%, #d4a5f9 100%);
      color: #5a2d82;
    }
    
    .btn-power-medium {
      background: linear-gradient(135deg, #b794f6 0%, #9b72f5 100%);
      color: white;
    }
    
    .btn-power-high {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
    }
    
    .btn-power:hover {
      transform: translateY(-2px);
      box-shadow: 0 6px 20px rgba(0,0,0,0.2);
    }
    
    .stats {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 15px;
      margin-top: 25px;
    }
    
    .stat-box {
      background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
      padding: 20px;
      border-radius: 12px;
      text-align: center;
    }
    
    .stat-label {
      font-size: 0.85em;
      color: #666;
      margin-bottom: 8px;
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }
    
    .stat-value {
      font-size: 2.2em;
      font-weight: bold;
      color: #667eea;
    }
    
    .tag-display {
      background: linear-gradient(135deg, #ffffff 0%, #f0f0f0 100%);
      border-radius: 15px;
      padding: 25px;
      margin-top: 25px;
    }
    
    .tag-display h3 {
      color: #555;
      margin-bottom: 15px;
      font-size: 1.2em;
    }
    
    table {
      width: 100%;
      border-collapse: collapse;
      background: white;
      border-radius: 8px;
      overflow: hidden;
    }
    
    thead {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
    }
    
    th {
      padding: 12px 8px;
      text-align: left;
      font-weight: 600;
      font-size: 0.9em;
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }
    
    td {
      padding: 10px 8px;
      border-bottom: 1px solid #f0f0f0;
      font-size: 0.85em;
    }
    
    tbody tr:hover {
      background: #f9f9f9;
    }
    
    .mono {
      font-family: 'Courier New', Monaco, monospace;
    }
    
    .footer {
      text-align: center;
      color: white;
      margin-top: 30px;
      opacity: 0.8;
      font-size: 0.9em;
    }
    
    .modal {
      display: none;
      position: fixed;
      z-index: 1000;
      left: 0;
      top: 0;
      width: 100%;
      height: 100%;
      background-color: rgba(0,0,0,0.5);
      animation: fadeIn 0.3s;
    }
    
    @keyframes fadeIn {
      from { opacity: 0; }
      to { opacity: 1; }
    }
    
    .modal-content {
      background: white;
      margin: 10% auto;
      padding: 40px;
      border-radius: 20px;
      max-width: 500px;
      box-shadow: 0 20px 60px rgba(0,0,0,0.3);
      animation: slideIn 0.3s;
    }
    
    @keyframes slideIn {
      from { transform: translateY(-50px); opacity: 0; }
      to { transform: translateY(0); opacity: 1; }
    }
    
    .modal-header {
      font-size: 1.5em;
      font-weight: bold;
      margin-bottom: 20px;
      color: #667eea;
      text-align: center;
    }
    
    .modal-body {
      margin: 20px 0;
    }
    
    .progress-bar {
      width: 100%;
      height: 35px;
      background: #f0f0f0;
      border-radius: 17px;
      overflow: hidden;
      margin: 20px 0;
      box-shadow: inset 0 2px 5px rgba(0,0,0,0.1);
    }
    
    .progress-fill {
      height: 100%;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      transition: width 0.3s;
      display: flex;
      align-items: center;
      justify-content: center;
      color: white;
      font-weight: bold;
      font-size: 0.95em;
    }
    
    input[type="text"] {
      width: 100%;
      padding: 15px;
      border: 2px solid #e0e0e0;
      border-radius: 10px;
      font-size: 16px;
      margin-top: 10px;
      transition: border-color 0.3s;
    }
    
    input[type="text"]:focus {
      outline: none;
      border-color: #667eea;
    }
    
    .modal-buttons {
      display: flex;
      gap: 15px;
      margin-top: 25px;
    }
    
    .modal-buttons button {
      flex: 1;
      padding: 15px;
      border: none;
      border-radius: 10px;
      font-size: 16px;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.3s;
    }
    
    .btn-modal-confirm {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
    }
    
    .btn-modal-confirm:hover {
      transform: scale(1.02);
    }
    
    .btn-modal-cancel {
      background: #f0f0f0;
      color: #666;
    }
    
    .btn-modal-cancel:hover {
      background: #e0e0e0;
    }
    
    .tag-name-display {
      font-weight: bold;
      color: #667eea;
    }
    
    .instruction-text {
      text-align: center;
      color: #666;
      margin-bottom: 15px;
      line-height: 1.5;
    }
    
    .epc-display {
      text-align: center;
      margin-top: 15px;
      padding: 10px;
      background: #f9f9f9;
      border-radius: 8px;
    }
    
    .epc-label {
      font-size: 0.85em;
      color: #999;
      margin-bottom: 5px;
    }
    
    .epc-value {
      font-family: 'Courier New', monospace;
      font-size: 0.9em;
      color: #667eea;
      font-weight: bold;
    }
    
    .info-box {
      margin-top: 20px;
      padding: 15px;
      background: #f9f9f9;
      border-radius: 10px;
    }
    
    .info-title {
      font-size: 0.9em;
      color: #666;
      margin-bottom: 10px;
      font-weight: 600;
    }
    
    .info-content {
      font-size: 0.85em;
      color: #999;
      line-height: 1.6;
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>RFID Powder Tracking</h1>
      <p>Metal Powder Inventory Management</p>
    </div>
    
    <div class="card">
      <div class="status-row">
        <div class="status-left">
          <span class="status-indicator" id="statusDot"></span>
          <span class="status-text" id="statusText">Initializing...</span>
        </div>
        <div class="history-badge" id="historyBadge">0 reads logged</div>
      </div>
      
      <div class="controls">
        <button class="btn btn-start" onclick="startScan()">
          Start Scan
        </button>
        <button class="btn btn-stop" onclick="stopScan()">
          Stop Scan
        </button>
        <button class="btn btn-clear" onclick="clearTags()">
          Clear
        </button>
        <button class="btn btn-register" onclick="startRegistration()">
          Register
        </button>
        <button class="btn btn-export" onclick="showExportDialog()">
          Export CSV
        </button>
      </div>
      
      <div class="section">
        <h3>Power Level Control</h3>
        <div class="power-buttons">
          <button class="btn-power btn-power-low" onclick="setPower(2000)">
            Low<br><small>20 dBm</small>
          </button>
          <button class="btn-power btn-power-medium" onclick="setPower(2600)">
            Medium<br><small>26 dBm</small>
          </button>
          <button class="btn-power btn-power-high" onclick="setPower(3000)">
            Maximum<br><small>30 dBm</small>
          </button>
        </div>
      </div>
      
      <div class="stats">
        <div class="stat-box">
          <div class="stat-label">Unique Bottles</div>
          <div class="stat-value" id="tagCount">0</div>
        </div>
        <div class="stat-box">
          <div class="stat-label">Power (dBm)</div>
          <div class="stat-value" id="powerDisplay">30.0</div>
        </div>
      </div>
      
      <div class="tag-display">
        <h3>Powder Inventory</h3>
        <div style="overflow-x: auto;">
          <table>
            <thead>
              <tr>
                <th style="width: 40px;">No.</th>
                <th>Bottle Name</th>
                <th>EPC</th>
                <th style="width: 80px; text-align: right;">RSSI</th>
                <th style="width: 60px; text-align: right;">CNT</th>
              </tr>
            </thead>
            <tbody id="tagTableBody">
              <tr><td colspan="5" style="text-align: center; padding: 20px; color: #999;">No bottles detected...</td></tr>
            </tbody>
          </table>
        </div>
      </div>
    </div>
    
    <div class="footer">
      <p>ESP32 RFID Powder Tracking System &bull; Logging up to 1000 reads</p>
    </div>
  </div>
  
  <!-- Registration Modal -->
  <div id="registrationModal" class="modal">
    <div class="modal-content">
      <div class="modal-header">Register New Bottle</div>
      <div class="modal-body">
        <div id="stepDetecting" style="display:block;">
          <p class="instruction-text">
            Place the RFID tag close to the reader<br>
            and <strong>hold it steady</strong> until confirmed...
          </p>
          <div class="progress-bar">
            <div class="progress-fill" id="progressBar" style="width: 0%;">
              <span id="progressText">0/5</span>
            </div>
          </div>
          <div class="epc-display">
            <div class="epc-label">Detected Tag:</div>
            <div class="epc-value" id="detectedEPC">Waiting...</div>
          </div>
        </div>
        
        <div id="stepNaming" style="display:none;">
          <p class="instruction-text" style="margin-bottom: 15px;">
            ✓ Tag detected successfully!<br>
            Give it a name:
          </p>
          <p style="margin-bottom: 10px; color: #999; font-size: 0.85em; text-align: center;">
            Examples: "Ti64 Batch #42", "AlSi10Mg 2024-02", "316L Powder A"
          </p>
          <input type="text" id="bottleName" placeholder="Enter bottle name..." maxlength="50">
          <div class="epc-display" style="margin-top: 15px;">
            <div class="epc-label">Tag ID:</div>
            <div class="epc-value" id="finalEPC">-</div>
          </div>
        </div>
      </div>
      <div class="modal-buttons">
        <button class="btn-modal-cancel" onclick="cancelRegistration()">Cancel</button>
        <button class="btn-modal-confirm" id="confirmButton" onclick="confirmRegistration()" style="display:none;">Save Bottle</button>
      </div>
    </div>
  </div>
  
  <!-- CSV Export Modal -->
  <div id="exportModal" class="modal">
    <div class="modal-content">
      <div class="modal-header">Export Test Data</div>
      <div class="modal-body">
        <p class="instruction-text">
          Name your test session to identify this data export
        </p>
        <p style="margin-bottom: 10px; color: #999; font-size: 0.85em; text-align: center;">
          Examples: "Level-1-Front", "Level-2-Back", "Level-3-Center"
        </p>
        <input type="text" id="testLocation" placeholder="e.g., Level-1-Front-Shelf" maxlength="50">
        <div class="info-box">
          <div class="info-title">Export will include:</div>
          <div class="info-content">
            • Every single tag read with timestamp<br>
            • RSSI value for each read<br>
            • Perfect for analyzing signal fluctuation<br>
            • Current logged reads: <strong id="exportReadCount">0</strong>
          </div>
        </div>
      </div>
      <div class="modal-buttons">
        <button class="btn-modal-cancel" onclick="cancelExport()">Cancel</button>
        <button class="btn-modal-confirm" onclick="confirmExport()">Download CSV</button>
      </div>
    </div>
  </div>
  
  <script>
    let registrationEPC = '';
    let currentTagData = [];
    
    function updateStatus() {
      fetch('/api/status')
        .then(response => response.json())
        .then(data => {
          currentTagData = data.tags || [];
          
          const statusText = data.scanning ? 'Scanning Active' : 'Idle';
          document.getElementById('statusText').textContent = statusText;
          
          const statusDot = document.getElementById('statusDot');
          statusDot.className = 'status-indicator ' + (data.scanning ? 'active' : 'inactive');
          
          // Update history badge
          const historyCount = data.historyCount || 0;
          document.getElementById('historyBadge').textContent = historyCount + ' reads logged';
          
          document.getElementById('tagCount').textContent = data.tagCount;
          document.getElementById('powerDisplay').textContent = (data.power / 100).toFixed(1);
          
          if (data.registrationMode) {
            const progress = Math.min(data.registrationProgress, 5);
            const percent = (progress / 5) * 100;
            document.getElementById('progressBar').style.width = percent + '%';
            document.getElementById('progressText').textContent = progress + '/5';
            
            if (data.registrationEPC) {
              const shortEPC = data.registrationEPC.substring(0, 20) + '...';
              document.getElementById('detectedEPC').textContent = shortEPC;
            }
            
            if (progress >= 5 && data.registrationEPC) {
              registrationEPC = data.registrationEPC;
              document.getElementById('stepDetecting').style.display = 'none';
              document.getElementById('stepNaming').style.display = 'block';
              document.getElementById('confirmButton').style.display = 'block';
              document.getElementById('finalEPC').textContent = data.registrationEPC.substring(0, 20) + '...';
              document.getElementById('bottleName').focus();
            }
          }
          
          const tableBody = document.getElementById('tagTableBody');
          if (data.tags && data.tags.length > 0) {
            let html = '';
            data.tags.forEach(tag => {
              html += '<tr>';
              html += '<td>' + tag.no + '</td>';
              
              if (tag.name && tag.name.length > 0) {
                html += '<td class="tag-name-display">' + tag.name + '</td>';
              } else {
                html += '<td style="color: #999; font-style: italic;">Unregistered</td>';
              }
              
              html += '<td class="mono" style="font-size: 0.75em;">' + tag.epc + '</td>';
              html += '<td style="text-align: right;">' + tag.rssi + ' dBm</td>';
              html += '<td style="text-align: right;">' + tag.cnt + '</td>';
              html += '</tr>';
            });
            tableBody.innerHTML = html;
          } else {
            tableBody.innerHTML = '<tr><td colspan="5" style="text-align: center; padding: 20px; color: #999;">No bottles detected...</td></tr>';
          }
        })
        .catch(err => {
          console.error('Error fetching status:', err);
          document.getElementById('statusText').textContent = 'Connection Error';
        });
    }
    
    function startScan() {
      fetch('/api/start')
        .then(() => updateStatus())
        .catch(err => console.error('Error starting scan:', err));
    }
    
    function stopScan() {
      fetch('/api/stop')
        .then(() => updateStatus())
        .catch(err => console.error('Error stopping scan:', err));
    }
    
    function setPower(powerValue) {
      fetch('/api/power?value=' + powerValue)
        .then(() => updateStatus())
        .catch(err => console.error('Error setting power:', err));
    }
    
    function clearTags() {
      if (confirm('Clear all detected tags and reading history?\n\n(Registered names will be kept)')) {
        fetch('/api/clear')
          .then(() => updateStatus())
          .catch(err => console.error('Error clearing tags:', err));
      }
    }
    
    function startRegistration() {
      document.getElementById('stepDetecting').style.display = 'block';
      document.getElementById('stepNaming').style.display = 'none';
      document.getElementById('confirmButton').style.display = 'none';
      document.getElementById('bottleName').value = '';
      document.getElementById('progressBar').style.width = '0%';
      document.getElementById('progressText').textContent = '0/5';
      document.getElementById('detectedEPC').textContent = 'Waiting...';
      registrationEPC = '';
      
      document.getElementById('registrationModal').style.display = 'block';
      
      fetch('/api/register/start')
        .catch(err => console.error('Error starting registration:', err));
    }
    
    function cancelRegistration() {
      document.getElementById('registrationModal').style.display = 'none';
      fetch('/api/register/cancel')
        .catch(err => console.error('Error cancelling registration:', err));
    }
    
    function confirmRegistration() {
      const name = document.getElementById('bottleName').value.trim();
      
      if (!name) {
        alert('Please enter a bottle name!');
        document.getElementById('bottleName').focus();
        return;
      }
      
      if (!registrationEPC) {
        alert('No tag detected!');
        return;
      }
      
      fetch('/api/register/confirm?name=' + encodeURIComponent(name) + '&epc=' + encodeURIComponent(registrationEPC))
        .then(() => {
          document.getElementById('registrationModal').style.display = 'none';
          alert('Bottle "' + name + '" registered successfully!');
          updateStatus();
        })
        .catch(err => console.error('Error confirming registration:', err));
    }
    
    function showExportDialog() {
      fetch('/api/history')
        .then(response => response.json())
        .then(data => {
          if (!data.readings || data.readings.length === 0) {
            alert('No reading history to export!\n\nStart scanning and detect some tags first.');
            return;
          }
          
          document.getElementById('testLocation').value = '';
          document.getElementById('exportReadCount').textContent = data.count;
          document.getElementById('exportModal').style.display = 'block';
          document.getElementById('testLocation').focus();
        })
        .catch(err => {
          console.error('Error checking history:', err);
          alert('Error loading history data!');
        });
    }
    
    function cancelExport() {
      document.getElementById('exportModal').style.display = 'none';
    }
    
    function confirmExport() {
      const location = document.getElementById('testLocation').value.trim();
      
      if (!location) {
        alert('Please enter a test location/name!');
        document.getElementById('testLocation').focus();
        return;
      }
      
      fetch('/api/history')
        .then(response => response.json())
        .then(data => {
          if (!data.readings || data.readings.length === 0) {
            alert('No reading history to export!');
            return;
          }
          
          const now = new Date();
          const dateStr = now.toISOString().substring(0, 10);
          const timeStr = now.toTimeString().substring(0, 8).replace(/:/g, '-');
          
          let csv = 'Test Location,Read Number,Timestamp,Bottle Name,EPC,RSSI (dBm)\n';
          
          data.readings.forEach((reading, index) => {
            const name = reading.name || 'Unregistered';
            csv += `"${location}",${index + 1},"${reading.time}","${name}","${reading.epc}",${reading.rssi}\n`;
          });
          
          const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
          const link = document.createElement('a');
          const url = URL.createObjectURL(blob);
          
          const filename = `RFID_${location.replace(/\s+/g, '_')}_${dateStr}_${timeStr}.csv`;
          
          link.setAttribute('href', url);
          link.setAttribute('download', filename);
          link.style.visibility = 'hidden';
          document.body.appendChild(link);
          link.click();
          document.body.removeChild(link);
          
          document.getElementById('exportModal').style.display = 'none';
          alert(`CSV exported successfully!\n\nFile: ${filename}\nTotal reads: ${data.readings.length}`);
        })
        .catch(err => {
          console.error('Error exporting:', err);
          alert('Error exporting data!');
        });
    }
    
    setInterval(updateStatus, 1000);
    updateStatus();
  </script>
</body>
</html>
)rawliteral";

#endif