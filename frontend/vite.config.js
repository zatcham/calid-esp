import { defineConfig } from 'vite';
import preact from '@preact/preset-vite';
import path from 'path';

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [preact()],
  base: './', // Use relative paths for assets
  build: {
    outDir: '../data',
    emptyOutDir: true,
  },
  server: {
    proxy: {
      '/api': 'http://192.168.4.1',
    }
  }
});
