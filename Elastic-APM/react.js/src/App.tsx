import React from 'react';
import { BrowserRouter, Routes, Route } from 'react-router-dom';
import { ApmRoute } from '@elastic/apm-rum-react';
import logo from './logo.svg';
import './App.css';
import Home from './pages/home';
import About from './pages/about';

function App() {
  return (
    <div className="App">
      <BrowserRouter>
        <Routes>
          <ApmRoute path="/" element={<Home />}></ApmRoute>
          <Route path="/about" element={<About />}></Route>
        </Routes>
      </BrowserRouter>
    </div>
  );
}

export default App;
