import { Router } from 'preact-router';
import { Header } from './components/Header';
import { Dashboard } from './pages/Dashboard';
import { Config } from './pages/Config';
import { Logs } from './pages/Logs';

// Import Bootstrap CSS
import 'bootstrap/dist/css/bootstrap.min.css';

export function App() {
  return (
    <div id="app">
      <Header />
      <Router>
        <Dashboard path="/" />
        <Config path="/config" />
        <Logs path="/logs" />
      </Router>
    </div>
  );
}