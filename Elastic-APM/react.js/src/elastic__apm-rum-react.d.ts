declare module '@elastic/apm-rum-react' {
    import { ComponentType } from 'react';
    import { Route } from 'react-router';
    export const ApmRoute: typeof Route;

    /**
     * Wrap a component to record an elastic APM transaction while it is rendered.
     *
     * Usage:
     *  - Pure function: `withTransaction('name','route-change')(Component)`
     *  - As a decorator: `@withTransaction('name','route-change')`
     */
    export const withTransaction: (
        name: string,
        eventType: string,
    ) => <T>(component: ComponentType<T>) => ComponentType<T>;
}