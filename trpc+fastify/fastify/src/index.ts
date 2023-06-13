import { fastifyTRPCPlugin } from '@trpc/server/adapters/fastify';
import fastify from 'fastify';
import { appRouter } from './router';


const server = fastify({
    maxParamLength: 4096,
});

server.get('/healthz', async (request, reply) => {
    return 'OK'
})

server.register(fastifyTRPCPlugin, {
    prefix: '/trpc',
    trpcOptions: { router: appRouter },
});

(async () => {
    try {
        await server.listen({ port: 8080 });
    } catch (err) {
        server.log.error(err);
        process.exit(1);
    }
})();
