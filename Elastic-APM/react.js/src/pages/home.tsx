import React from 'react';
import logo from "../logo.svg";
import {Link} from "react-router-dom";

function Home() {
    return (
        <header className="App-header">
            <img src={logo} className="App-logo" alt="logo" />
            <p>
                Edit <code>src/App.tsx</code> and save to reload.
            </p>
            <a
                className="App-link"
                href="https://reactjs.org"
                target="_blank"
                rel="noopener noreferrer"
            >
                Learn React
            </a>

            <div style={{backgroundColor: 'white'}}>
                <h1>Home</h1>
                <ul>
                    <li><Link to="/">Home</Link></li>
                    <li><Link to="/test">Test</Link></li>
                    <li><Link to="/about">About</Link></li>
                </ul>
            </div>
        </header>
    )
}

export default Home;
