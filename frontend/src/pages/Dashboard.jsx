import { useEffect, useState } from 'preact/hooks';
import { api } from '../api';

export function Dashboard() {
  const [data, setData] = useState({ sensors: [] });
  const [system, setSystem] = useState(null);
  const [loading, setLoading] = useState(true);

  const fetchData = async () => {
    const res = await api.getCurrentData();
    if (res && res.sensors) {
      setData(res);
    }
    setLoading(false);
  };

  const fetchSystemInfo = async () => {
    const res = await api.getSystemInfo();
    if (res) {
      setSystem(res);
    }
  };

  useEffect(() => {
    fetchData();
    fetchSystemInfo();
    const interval = setInterval(fetchData, 5000); // Poll every 5s
    return () => clearInterval(interval);
  }, []);

  return (
    <div class="container pb-5">
      <div class="d-flex justify-content-between align-items-center mb-4">
        <h2>Live Monitor</h2>
        {system && (
          <div class="text-muted text-end">
            <div>Adoption Code: <strong class="text-primary">{system.adoptionCode}</strong></div>
            <small class="text-secondary">{system.macAddress}</small>
          </div>
        )}
      </div>
      
      {loading ? (
        <div class="alert alert-info">Loading sensor data...</div>
      ) : (
        <div class="row">
          {data.sensors.length === 0 && (
              <div class="col-12">
                  <div class="alert alert-warning shadow-sm">No sensors active. Check hardware configuration.</div>
              </div>
          )}
          
          {data.sensors.map((sensor, index) => (
            <div class="col-12 mb-4" key={index}>
              <div class="card shadow-sm">
                <div class="card-header bg-white d-flex justify-content-between align-items-center">
                  <div>
                    <span class="h5 mb-0">{sensor.sensorType}</span>
                    <span class="badge bg-light text-dark ms-2 border">Pin {sensor.pin}</span>
                  </div>
                  <span class={`badge ${sensor.valid ? 'bg-success' : 'bg-danger'}`}>
                    {sensor.valid ? 'ONLINE' : 'OFFLINE'}
                  </span>
                </div>
                <div class="card-body">
                  {!sensor.valid && sensor.error && (
                      <div class="alert alert-danger py-2 mb-3 small">{sensor.error}</div>
                  )}
                  
                  <div class="row">
                    {sensor.readings.map((reading, rIdx) => (
                      <div class="col-md-4 mb-3" key={rIdx}>
                        <div class="text-center p-3 border rounded bg-light">
                          <h6 class="text-muted small text-uppercase fw-bold">{reading.type}</h6>
                          <div class="d-flex align-items-baseline justify-content-center">
                            <span class="display-6">{reading.value.toFixed(1)}</span>
                            <span class="ms-1 text-muted">{reading.unit}</span>
                          </div>
                        </div>
                      </div>
                    ))}
                  </div>
                </div>
              </div>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}
