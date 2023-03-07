import { init as initApm } from '@elastic/apm-rum'

const apm = initApm({
    serviceName: 'tutorial-nextjs-csr',
    serverUrl: 'http://localhost:8200',
    serviceVersion: 'v0.0.1',
    environment: 'localhost'
})

export default apm
