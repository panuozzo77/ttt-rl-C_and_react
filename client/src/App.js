import React from 'react';
import { BrowserRouter as Router, Route, Routes, Link, NavLink } from 'react-router-dom';
import GamePage from './pages/GamePage';
import TrainingPage from './pages/TrainingPage';
import './App.css'; // We'll create this for basic styling

function App() {
  return (
    <Router>
      <div className="App">
        <header>
          <h1>Tic-Tac-Toe AI</h1>
          <nav>
            <ul>
              <li><NavLink to="/" className={({ isActive }) => isActive ? "active-link" : ""}>Play Game</NavLink></li>
              <li><NavLink to="/training" className={({ isActive }) => isActive ? "active-link" : ""}>Training Stats</NavLink></li>
            </ul>
          </nav>
        </header>
        <main>
          <Routes>
            <Route path="/" element={<GamePage />} />
            <Route path="/training" element={<TrainingPage />} />
          </Routes>
        </main>
        <footer>
          <p>Tic-Tac-Toe RL Interface</p>
        </footer>
      </div>
    </Router>
  );
}

export default App;