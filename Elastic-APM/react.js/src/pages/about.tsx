import React from 'react';
import logo from "../logo.svg";
import {Link} from "react-router-dom";

function About() {
    return (
        <div>
            <h1>About</h1>
            <ul>
                <li><Link to="/">Home</Link></li>
                <li><Link to="/test">Test</Link></li>
                <li><Link to="/about">About</Link></li>
            </ul>
        </div>
    )
}

export default About;
