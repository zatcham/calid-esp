import { useEffect, useState } from 'preact/hooks';
import { api } from '../api';

export function Config() {
  const [config, setConfig] = useState(null);
  const [loading, setLoading] = useState(true);
  const [saving, setSaving] = useState(false);

  useEffect(() => {
    api.getCurrentConfig().then(data => {
      if (data) {
          // Ensure sensors array exists
          if (!data.sensors) {
              data.sensors = Array(4).fill({ type: 0, pin: 0 });
          }
          setConfig(data);
      }
      setLoading(false);
    });
  }, []);

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
          newSensors[index] = { ...newSensors[index], [field]: parseInt(value) };
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
    
    // Map sensors to flat fields for backend handleApiConfigSave
    config.sensors.forEach((s, i) => {
        submission[`sensorType${i}`] = s.type;
        submission[`sensorPin${i}`] = s.pin;
    });

    // Handle checkboxes
    submission.testingMode = config.testingMode ? "on" : "off";
    submission.mqttEnabled = config.mqttEnabled ? "on" : "off";

    const success = await api.saveConfig(submission);
    if (success) {
      alert("Configuration saved! Device is restarting...");
    } else {
      alert("Failed to save configuration.");
    }
    setSaving(false);
  };

  if (loading) return <div class="container">Loading...</div>;

  return (
    <div class="container pb-5">
      <h2 class="mb-4">Configuration</h2>
      <form onSubmit={handleSubmit}>
        
        <div class="card shadow-sm mb-4">
            <div class="card-header bg-primary text-white">WiFi & API</div>
            <div class="card-body">
                <div class="row">
                    <div class="col-md-6 mb-3">
                        <label class="form-label">SSID</label>
                        <input type="text" class="form-control" name="ssid" value={config.ssid} onInput={handleChange} />
                    </div>
                    <div class="col-md-6 mb-3">
                        <label class="form-label">Password</label>
                        <input type="password" class="form-control" name="password" value={config.password} onInput={handleChange} />
                    </div>
                </div>
                <div class="mb-3">
                    <label class="form-label">API Endpoint</label>
                    <input type="text" class="form-control" name="apiEndpoint" value={config.apiEndpoint} onInput={handleChange} />
                </div>
                <div class="row">
                    <div class="col-md-6 mb-3">
                        <label class="form-label">Sensor ID</label>
                        <input type="text" class="form-control" name="sensorId" value={config.sensorId} onInput={handleChange} />
                    </div>
                    <div class="col-md-6 mb-3">
                        <label class="form-label">API Key</label>
                        <input type="text" class="form-control" name="apiKey" value={config.apiKey} onInput={handleChange} />
                    </div>
                </div>
            </div>
        </div>

        <div class="card shadow-sm mb-4">
            <div class="card-header bg-success text-white">Sensor Hardware</div>
            <div class="card-body">
                <p class="text-muted small">Configure up to 4 sensors on different GPIO pins.</p>
                {config.sensors.map((sensor, i) => (
                    <div class="row mb-3 pb-3 border-bottom align-items-end" key={i}>
                        <div class="col-md-1 mb-2"><strong>#{i+1}</strong></div>
                        <div class="col-md-6 mb-2">
                            <label class="form-label small">Sensor Type</label>
                            <select class="form-select" value={sensor.type} onChange={(e) => handleSensorChange(i, 'type', e.target.value)}>
                                <option value="0">Disabled</option>
                                <option value="11">DHT11</option>
                                <option value="22">DHT22</option>
                                <option value="2">BME280 (I2C)</option>
                            </select>
                        </div>
                        <div class="col-md-5 mb-2">
                            <label class="form-label small">GPIO Pin</label>
                            <input type="number" class="form-control" value={sensor.pin} onInput={(e) => handleSensorChange(i, 'pin', e.target.value)} placeholder="e.g. 4" />
                        </div>
                    </div>
                ))}
            </div>
        </div>

        <div class="card shadow-sm mb-4">
            <div class="card-header bg-info text-white">MQTT Settings</div>
            <div class="card-body">
                <div class="form-check mb-3">
                    <input class="form-check-input" type="checkbox" name="mqttEnabled" checked={config.mqttEnabled} onChange={handleCheckboxChange} />
                    <label class="form-check-label">Enable MQTT</label>
                </div>
                {config.mqttEnabled && (
                    <div class="row">
                        <div class="col-md-8 mb-3">
                            <label class="form-label">Broker</label>
                            <input type="text" class="form-control" name="mqttBroker" value={config.mqttBroker} onInput={handleChange} />
                        </div>
                        <div class="col-md-4 mb-3">
                            <label class="form-label">Port</label>
                            <input type="number" class="form-control" name="mqttPort" value={config.mqttPort} onInput={handleChange} />
                        </div>
                        <div class="col-md-6 mb-3">
                            <label class="form-label">User</label>
                            <input type="text" class="form-control" name="mqttUser" value={config.mqttUser} onInput={handleChange} />
                        </div>
                        <div class="col-md-6 mb-3">
                            <label class="form-label">Password</label>
                            <input type="password" class="form-control" name="mqttPassword" value={config.mqttPassword} onInput={handleChange} />
                        </div>
                        <div class="mb-3">
                            <label class="form-label">Topic Prefix</label>
                            <input type="text" class="form-control" name="mqttTopicPrefix" value={config.mqttTopicPrefix} onInput={handleChange} />
                        </div>
                    </div>
                )}
            </div>
        </div>

        <div class="card shadow-sm mb-4 border-warning">
            <div class="card-header bg-warning">System</div>
            <div class="card-body">
                <div class="form-check">
                    <input class="form-check-input" type="checkbox" name="testingMode" checked={config.testingMode} onChange={handleCheckboxChange} />
                    <label class="form-check-label">Testing Mode (Simulate multi-sensor data)</label>
                </div>
            </div>
        </div>

        <button type="submit" class="btn btn-primary btn-lg w-100 shadow" disabled={saving}>
            {saving ? 'Saving...' : 'Save Configuration & Restart'}
        </button>
      </form>
    </div>
  );
}