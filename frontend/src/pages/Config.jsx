import { useEffect, useState } from 'preact/hooks';
import { api } from '../api';

export function Config() {
  const [config, setConfig] = useState(null);
  const [loading, setLoading] = useState(true);
  const [saving, setSaving] = useState(false);
  const [i2cDevices, setI2cDevices] = useState([]);
  const [scanning, setScanning] = useState(false);

  useEffect(() => {
    api.getCurrentConfig().then(data => {
      if (data) {
          if (!data.sensors) {
              data.sensors = Array(4).fill({ type: 0, pin: 0, i2cAddress: 0x76 });
          }
          setConfig(data);
      }
      setLoading(false);
    });
  }, []);

  const scanI2C = async () => {
    setScanning(true);
    try {
        const res = await fetch('/api/system/scan-i2c');
        const addresses = await res.json();
        setI2cDevices(addresses);
    } catch (e) {
        console.error("Scan failed", e);
    }
    setScanning(false);
  };

  const handleChange = (e) => {
    const { name, value, type, checked } = e.target;
    setConfig(prev => ({
      ...prev,
      [name]: type === 'checkbox' ? checked : value
    }));
  };

  const handleSensorChange = (index, field, value) => {
      setConfig(prev => {
          const newSensors = [...prev.sensors];
          let val = value;
          if (field === 'pin' || field === 'type') val = parseInt(value);
          newSensors[index] = { ...newSensors[index], [field]: val };
          return { ...prev, sensors: newSensors };
      });
  };
  
  const handleCheckboxChange = (e) => {
      const { name, checked } = e.target;
      setConfig(prev => ({
          ...prev,
          [name]: checked
      }));
  };

  const handleSubmit = async (e) => {
    e.preventDefault();
    setSaving(true);
    
    const submission = { ...config };
    config.sensors.forEach((s, i) => {
        submission[`sensorType${i}`] = s.type;
        submission[`sensorPin${i}`] = s.pin;
        submission[`sensorI2C${i}`] = s.i2cAddress;
    });

    submission.testingMode = config.testingMode ? "on" : "off";
    submission.mqttEnabled = config.mqttEnabled ? "on" : "off";

    const success = await api.saveConfig(submission);
    if (success) {
      alert("Configuration saved! Device is restarting...");
    } else {
      alert("Failed to save configuration. Check credentials.");
    }
    setSaving(false);
  };

  if (loading) return <div class="container py-5">Loading configuration...</div>;

  return (
    <div class="container pb-5">
      <h2 class="mb-4">System Configuration</h2>
      <form onSubmit={handleSubmit}>
        
        <div class="card shadow-sm mb-4">
            <div class="card-header bg-primary text-white">Network & API</div>
            <div class="card-body">
                <div class="row">
                    <div class="col-md-6 mb-3">
                        <label class="form-label">WiFi SSID</label>
                        <input type="text" class="form-control" name="ssid" value={config.ssid} onInput={handleChange} />
                    </div>
                    <div class="col-md-6 mb-3">
                        <label class="form-label">WiFi Password</label>
                        <input type="password" class="form-control" name="password" value={config.password} onInput={handleChange} />
                    </div>
                </div>
                <div class="mb-3">
                    <label class="form-label">Remote API Endpoint</label>
                    <input type="text" class="form-control" name="apiEndpoint" value={config.apiEndpoint} onInput={handleChange} placeholder="https://api.example.com" />
                </div>
                <div class="row">
                    <div class="col-md-6 mb-3">
                        <label class="form-label">Device Sensor ID</label>
                        <input type="text" class="form-control" name="sensorId" value={config.sensorId} onInput={handleChange} />
                    </div>
                    <div class="col-md-6 mb-3">
                        <label class="form-label">Device API Key</label>
                        <input type="text" class="form-control" name="apiKey" value={config.apiKey} onInput={handleChange} />
                    </div>
                </div>
            </div>
        </div>

        <div class="card shadow-sm mb-4">
            <div class="card-header bg-success text-white d-flex justify-content-between align-items-center">
                <span>Sensor Hardware</span>
                <button type="button" class="btn btn-sm btn-outline-light" onClick={scanI2C} disabled={scanning}>
                    {scanning ? 'Scanning...' : 'Scan I2C Bus'}
                </button>
            </div>
            <div class="card-body">
                {i2cDevices.length > 0 && (
                    <div class="alert alert-info py-2 small mb-3">
                        Detected I2C Addresses: {i2cDevices.map(addr => '0x' + addr.toString(16).toUpperCase()).join(', ')}
                    </div>
                )}
                <div class="table-responsive">
                    <table class="table table-sm align-middle">
                        <thead>
                            <tr>
                                <th>#</th>
                                <th>Type</th>
                                <th>GPIO Pin</th>
                                <th>I2C Address</th>
                            </tr>
                        </thead>
                        <tbody>
                            {config.sensors.map((sensor, i) => (
                                <tr key={i}>
                                    <td>{i+1}</td>
                                    <td>
                                        <select class="form-select form-select-sm" value={sensor.type} onChange={(e) => handleSensorChange(i, 'type', e.target.value)}>
                                            <option value="0">Disabled</option>
                                            <option value="11">DHT11</option>
                                            <option value="22">DHT22</option>
                                            <option value="2">BME280</option>
                                        </select>
                                    </td>
                                    <td>
                                        <input type="number" class="form-control form-control-sm" value={sensor.pin} onInput={(e) => handleSensorChange(i, 'pin', e.target.value)} />
                                    </td>
                                    <td>
                                        <input type="text" class="form-control form-control-sm" value={sensor.i2cAddress} 
                                               disabled={sensor.type != 2}
                                               onInput={(e) => handleSensorChange(i, 'i2cAddress', e.target.value)} placeholder="0x76" />
                                    </td>
                                </tr>
                            ))}
                        </tbody>
                    </table>
                </div>
            </div>
        </div>

        <div class="row">
            <div class="col-md-6">
                <div class="card shadow-sm mb-4">
                    <div class="card-header bg-info text-white">Administration</div>
                    <div class="card-body">
                        <div class="mb-3">
                            <label class="form-label">Admin Username</label>
                            <input type="text" class="form-control" name="adminUser" value={config.adminUser} onInput={handleChange} />
                        </div>
                        <div class="mb-3">
                            <label class="form-label">New Admin Password</label>
                            <input type="password" class="form-control" name="adminPassword" value={config.adminPassword} onInput={handleChange} placeholder="Leave blank to keep current" />
                            <div class="form-text">Used for saving config and viewing logs.</div>
                        </div>
                    </div>
                </div>
            </div>
            <div class="col-md-6">
                <div class="card shadow-sm mb-4">
                    <div class="card-header bg-dark text-white">Time & System</div>
                    <div class="card-body">
                        <div class="mb-3">
                            <label class="form-label">NTP UTC Offset (seconds)</label>
                            <input type="number" class="form-control" name="utcOffset" value={config.utcOffset} onInput={handleChange} />
                            <div class="form-text">e.g. 3600 for UTC+1, -18000 for EST.</div>
                        </div>
                        <div class="form-check">
                            <input class="form-check-input" type="checkbox" name="testingMode" checked={config.testingMode} onChange={handleCheckboxChange} />
                            <label class="form-check-label text-danger fw-bold">Simulation Mode</label>
                        </div>
                    </div>
                </div>
            </div>
        </div>

        <div class="card shadow-sm mb-4">
            <div class="card-header bg-secondary text-white">MQTT Broker</div>
            <div class="card-body">
                <div class="form-check mb-3">
                    <input class="form-check-input" type="checkbox" name="mqttEnabled" checked={config.mqttEnabled} onChange={handleCheckboxChange} />
                    <label class="form-check-label">Enable MQTT Data Stream</label>
                </div>
                {config.mqttEnabled && (
                    <div class="row">
                        <div class="col-md-8 mb-3">
                            <label class="form-label">Broker Host</label>
                            <input type="text" class="form-control" name="mqttBroker" value={config.mqttBroker} onInput={handleChange} />
                        </div>
                        <div class="col-md-4 mb-3">
                            <label class="form-label">Port</label>
                            <input type="number" class="form-control" name="mqttPort" value={config.mqttPort} onInput={handleChange} />
                        </div>
                        <div class="col-md-6 mb-3">
                            <label class="form-label">Username</label>
                            <input type="text" class="form-control" name="mqttUser" value={config.mqttUser} onInput={handleChange} />
                        </div>
                        <div class="col-md-6 mb-3">
                            <label class="form-label">Password</label>
                            <input type="password" class="form-control" name="mqttPassword" value={config.mqttPassword} onInput={handleChange} />
                        </div>
                    </div>
                )}
            </div>
        </div>

        <button type="submit" class="btn btn-primary btn-lg w-100 shadow py-3 fw-bold" disabled={saving}>
            {saving ? 'RESTARTING DEVICE...' : 'APPLY CONFIGURATION'}
        </button>
      </form>
    </div>
  );
}
