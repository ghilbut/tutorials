Elastic-APM tutorial
====================

---

# 0. Prepare environment

## Run Database

```shell
$ cd $REPODIR/Elastic-APM/.local/db
$ docker-compose up -d
```

## Run Elastic-APM

```shell
$ cd $REPODIR/Elastic-APM/.local/es
$ docker-compose up -d
```

* access http://localhost:5601/ page
* Left menu → Observability / APM
* click `Add the APM integration` button on center of page
* click `Add Elasti-APM` button on right upper side of page
  * General
    * Host: **apm-server:8200**
    * URL: **http://apm-server:8200**

---

# 1. Dajngo

## 1a. Run Django server

```shell
$ cd $REPODIR/Elastic-APM/django
$ pipenv install
$ pipenv run ./manage.py makemigrations
$ pipenv run ./manage.py migrate
$ pipenv run ./manage.py createsuperuser
$ pipenv run ./manage.py runserver
```
## 1b. Edit [settings.py](django/tutorial/settings.py)

* **[[참고] APM Python Agent → Django support](https://www.elastic.co/guide/en/apm/agent/python/6.x/django-support.html)**

### [INSTALLED_APPS](django/tutorial/settings.py#L33)

```python
INSTALLED_APPS = [
    ...
    'elasticapm.contrib.django',  ## elastic-apm
]
```

### [MIDDLEWARE](django/tutorial/settings.py#L44)

```python
MIDDLEWARE = [
    ...
    'elasticapm.contrib.django.middleware.Catch404Middleware',  ## elastic-apm
]
```

### [TEMPLATES](django/tutorial/settings.py#L57)

```python
TEMPLATES = [
    {
        ...
        'OPTIONS': {
            'context_processors': [
                ...
                'elasticapm.contrib.django.context_processors.rum_tracing',  ## elastic-apm
            ],
        },
    },
]
```

### [LOGGING](django/tutorial/settings.py#L158)

```python
LOGGING = {
    'version': 1,
    'disable_existing_loggers': True,
    'formatters': {
        'verbose': {
            'format': '%(levelname)s %(asctime)s %(module)s %(process)d %(thread)d %(message)s'
        },
    },
    'handlers': {
        'elasticapm': {
            'level': 'WARNING',
            'class': 'elasticapm.contrib.django.handlers.LoggingHandler',
        },
        'console': {
            'level': 'DEBUG',
            'class': 'logging.StreamHandler',
            'formatter': 'verbose'
        }
    },
    'loggers': {
        'django': {
            'level': 'INFO',
            'handlers': ['console'],
            'propagate': True,
        },
        'django.db.backends': {
            'level': 'WARNING',
            'handlers': ['console'],
            'propagate': False,
        },
        # Log errors from the Elastic APM module to the console (recommended)
        'elasticapm.errors': {
            'level': 'WARNING',
            'handlers': ['console'],
            'propagate': False,
        },
        'k8single': {
            'level': 'DEBUG',
            'handlers': ['elasticapm'],
            'propagate': False,
        },
    },
}
```

### [ELASTIC_APM](django/tutorial/settings.py#L203)

* **[[참고] APM Python Agent → Configuration](https://www.elastic.co/guide/en/apm/agent/python/6.x/configuration.html)**

```python
ELASTIC_APM = {
    'DEBUG': True,
    'SERVICE_NAME': 'tutorial-django-server',
    'SERVER_URL': 'http://localhost:8200',
    'LOG_LEVEL': 'warning',
    'SERVICE_NODE_NAME': 'localhost',
    'ENVIRONMENT': 'ghilbut',  ## production, staging, develop, {{user}}
    'CLOUD_PROVIDER': 'none',
    'SERVICE_VERSION': 'v0.1',
    'TRANSACTION_IGNORE_URLS': ['/healthz', '/static/*', ],
    'TRANSACTIONS_IGNORE_PATTERNS': ['^OPTIONS '],
    'DJANGO_TRANSACTION_NAME_FROM_ROUTE': True,
}
```

## 1c. Edit base template for Elastic-APM RUM

* **[[참고] APM Python Agent → Django support](https://www.elastic.co/guide/en/apm/agent/python/6.x/django-support.html#django-integrating-with-the-rum-agent)**
* **[[참고] APM Real User Monitoring JavaScript Agent → Install the Agent](https://www.elastic.co/guide/en/apm/agent/rum-js/5.x/install-the-agent.html#using-script-tags)**

### Download and place [elastic-apm-rum.umd.min.js](django/static/elastic-apm-rum.umd.min.js)



### Edit [base.html](django/templates/base.html)

```html
<script src="/static/elastic-apm-rum.umd.min.js" crossorigin></script>
<script>
    elasticApm.init({
        serviceName: "tutorial-django-rum",
        serverUrl: "http://localhost:8200",
        pageLoadTraceId: "{{ apm.trace_id }}",
        pageLoadSpanId: "{{ apm.span_id }}",
        pageLoadSampled: {{ apm.is_sampled_js }}
    });
</script>
```

---

# 2. Next.js

## 2a. Create application

```shell
$ yarn create next-app --typescript ${PROJECT_NAME}
$ cd ${PROJECT_NAME}
$ yarn add elastic-apm-node
$ yarn add @elastic/apm-rum
$ yarn add @elastic/apm-rum-react
$ yarn add react-router-dom
```

## 2b. Activate Elastic-APM for SSR (server side rendering)

* **[[참고] APM Node.js Agent → Get started with Next.js](https://www.elastic.co/guide/en/apm/agent/nodejs/3.x/nextjs.html)**

### [package.json](next.js/package.json#L6)

```json
{
  "scripts": {
    "dev": "NODE_OPTIONS=--require=elastic-apm-node/start-next.js next dev",
    "build": "next build",
    "start": "NODE_OPTIONS=--require=elastic-apm-node/start-next.js next start",
    "lint": "next lint"
  }
}
```

### [elastic-apm-node.js](next.js/elastic-apm-node.js)

* **[[참고] APM Node.js Agent → Configuring the agent](https://www.elastic.co/guide/en/apm/agent/nodejs/3.x/configuring-the-agent.html)**
* **[[참고] APM Node.js Agent → Configuration options](https://www.elastic.co/guide/en/apm/agent/nodejs/3.x/configuration.html)**

```javascript
module.exports = {
  serviceName: 'tutorial-nextjs-ssr',
  serviceNodeName: 'localhost',
  serverUrl: 'http://localhost:8200',
  serviceVersion: 'v0.0.1',
  environment: 'jhkim'
}
```

## 2c. Activate Elastic-APM RUM for CSR (client side rendering)

* **[[참고] APM Real User Monitoring JavaScript Agent → Install the agent](https://www.elastic.co/guide/en/apm/agent/rum-js/5.x/install-the-agent.html#using-bundlers)**
* **[[참고] APM Real User Monitoring JavaScript Agent → Configuration](https://www.elastic.co/guide/en/apm/agent/rum-js/5.x/configuration.html)**

### [modules/apm.ts](next.js/modules/apm.ts)

```typescript
import { init as initApm } from '@elastic/apm-rum'

const apm = initApm({
    serviceName: 'tutorial-nextjs-csr',
    serviceNodeName: 'jhkim',
    serverUrl: 'http://localhost:8200',
    serviceVersion: 'v0.0.1',
    environment: 'localhost'
})

export default apm
```

### [pages/_app.tsx](next.js/_app.tsx#L3)

```typescript
import apm from '@/modules/apm'

console.log('Elastic-APM is activated: ', apm.isActive())
```

### [types/elastic__apm-rum-react.d.ts](next.js/types/elastic__apm-rum-react.d.ts) (only for typescript)

```typescript
// https://github.com/elastic/apm-agent-rum-js/issues/624#issuecomment-598492346
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
```

# 3. React.js

* **[[참고] APM Real User Monitoring JavaScript Agent → Install the Agent](https://www.elastic.co/guide/en/apm/agent/rum-js/5.x/install-the-agent.html#using-bundlers)**
* **[[참고] APM Real User Monitoring JavaScript Agent → Configuration](https://www.elastic.co/guide/en/apm/agent/rum-js/current/configuration.html)**
* **[[참고] APM Real User Monitoring JavaScript Agent → React integration](https://www.elastic.co/guide/en/apm/agent/rum-js/5.x/react-integration.html)**

## 3a. Create react.js application 

```shell
$ yarn create react-app ${NAME:-tutorial} --template typescript
$ cd ${NAME:-tutorial}
$ yarn add react-router-dom
$ yarn add @elastic/apm-rum
$ yarn add @elastic/apm-rum-react
```

## 3b. Set APM configuration

```shell
## ${PRJDIR}/src/App.tsx

import { init as initApm } from '@elastic/apm-rum'

const apm = initApm({
  serviceName: 'tutorial-reactjs',
  serverUrl: 'http://localhost:8200',
  serviceVersion: 'v0.1',
  environment: 'ghilbut',
});
```



## 3z. Run react.js application

```shell
$ yarn start
```
