
  updateFirmware: async (file) => {
    const formData = new FormData();
    formData.append("update", file);
    
    try {
      const res = await fetch(`${API_BASE}/update`, {
        method: 'POST',
        body: formData
      });
      return res.ok;
    } catch (e) {
      console.error("Error updating firmware:", e);
      return false;
    }
  }
