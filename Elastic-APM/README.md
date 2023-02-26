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
* **[[참고] APM Real User Monitoring JavaScript Agent → Install the Agent](https://www.elastic.co/guide/en/apm/agent/rum-js/5.x/install-the-agent.html)**

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