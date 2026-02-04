import { useEffect, useState } from 'preact/hooks';
import { api } from '../api';

export function Logs() {
  const [logs, setLogs] = useState('');

  useEffect(() => {
    api.getLogs().then(setLogs);
  }, []);

  return (
    <div class="container">
      <h2>Device Logs</h2>
      <pre class="bg-light p-3 border rounded" style={{ maxHeight: '600px', overflowY: 'scroll' }}>
        {logs}
      </pre>
      <button class="btn btn-secondary mt-2" onClick={() => window.location.reload()}>Refresh</button>
    </div>
  );
}
