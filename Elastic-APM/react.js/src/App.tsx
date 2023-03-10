import React from 'react';
import { BrowserRouter, Switch } from 'react-router-dom';
import { ApmRoute } from '@elastic/apm-rum-react';
import './App.css';
import Home from './pages/home';
import Test from "./pages/test";
import About from './pages/about';

function App() {
  return (
    <div className="App">
      <BrowserRouter>
        <Switch>
          <ApmRoute exact path="/" component={Home}></ApmRoute>
          <ApmRoute exact path="/test" component={Test}></ApmRoute>
          <ApmRoute exact path="/about" component={About}></ApmRoute>
        </Switch>
      </BrowserRouter>
    </div>
  );
}

export default App;
