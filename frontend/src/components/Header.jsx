import { Link } from 'preact-router/match';

export function Header() {
  return (
    <nav class="navbar navbar-expand navbar-dark bg-primary mb-4">
      <div class="container">
        <a class="navbar-brand" href="/">Calid</a>
        <div class="navbar-nav">
          <Link activeClassName="active" class="nav-link" href="/">Dashboard</Link>
          <Link activeClassName="active" class="nav-link" href="/config">Config</Link>
          <Link activeClassName="active" class="nav-link" href="/logs">Logs</Link>
        </div>
      </div>
    </nav>
  );
}
