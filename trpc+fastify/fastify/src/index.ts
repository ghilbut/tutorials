import fastify from 'fastify';
import cors from '@fastify/cors';
import { fastifyTRPCPlugin } from '@trpc/server/adapters/fastify';
import { appRouter } from './router';


const server = fastify({
    maxParamLength: 4096,
});

server.register(cors);
server.register(fastifyTRPCPlugin, {
    prefix: '/api/trpc',
    trpcOptions: { router: appRouter },
});

server.get('/healthz', async (request, reply) => {
    return 'OK'
});

(async () => {
    try {
        await server.listen({ port: 8080 });
        console.log('listening on port', 8080);
    } catch (err) {
        server.log.error(err);
        process.exit(1);
    }
})();
