import { withTransaction } from '@elastic/apm-rum-react'

function ApmConfigurations({ server, agent }: { server: any, agent: any }) {
    return (
        <>
            <ul>
                <li>
                    <h1>Server information</h1>
                    <ul>
                        <li>build_date: { server.build_date }</li>
                        <li>build_sha: { server.build_sha }</li>
                        <li>publish_ready: { server.publish_ready }</li>
                        <li>version: { server.version }</li>
                    </ul>
                </li>
                <li>
                    <h1>Agent configuration</h1>
                    <ul>
                        <li>error: { agent.error }</li>
                    </ul>
                </li>
            </ul>
        </>
    )
}

export default withTransaction('ApmConfigurations', 'component')(ApmConfigurations)