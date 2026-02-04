
const API_BASE = '/api'; // Relative path for production

export const api = {
  getCurrentData: async () => {
    try {
      const res = await fetch(`${API_BASE}/data`);
      if (!res.ok) throw new Error('Network response was not ok');
      return await res.json();
    } catch (e) {
      console.error("Error fetching data:", e);
      return null;
    }
  },

  getSystemInfo: async () => {
    try {
      const res = await fetch(`${API_BASE}/system`);
      if (!res.ok) throw new Error('Network response was not ok');
      return await res.json();
    } catch (e) {
      console.error("Error fetching system info:", e);
      return null;
    }
  },

  getCurrentConfig: async () => {
    try {
      const res = await fetch(`${API_BASE}/config`);
      if (!res.ok) throw new Error('Network response was not ok');
      return await res.json();
    } catch (e) {
      console.error("Error fetching config:", e);
      return null;
    }
  },

  getLogs: async () => {
    try {
      const res = await fetch(`${API_BASE}/logs`);
      return await res.text();
    } catch (e) {
      console.error("Error fetching logs:", e);
      return "Error fetching logs";
    }
  },

  saveConfig: async (configData) => {
    const params = new URLSearchParams();
    for (const key in configData) {
        if (configData[key] !== undefined && configData[key] !== null) {
             params.append(key, configData[key]);
        }
    }
    
    try {
      const res = await fetch(`${API_BASE}/config`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded',
        },
        body: params
      });
      return res.ok;
    } catch (e) {
      console.error("Error saving config:", e);
      return false;
    }
  }
};
