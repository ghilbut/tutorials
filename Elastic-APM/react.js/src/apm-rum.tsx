import {init as initApm} from "@elastic/apm-rum";

const apm = initApm({
  serviceName: 'tutorial-reactjs',
  serverUrl: 'http://localhost:8200',
  serviceVersion: 'v0.1',
  environment: 'ghilbut',
});

export default apm;
