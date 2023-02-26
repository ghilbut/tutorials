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

## Run Django server

```shell
$ cd $REPODIR/Elastic-APM/django
$ pipenv install
$ pipenv run ./manage.py makemigrations
$ pipenv run ./manage.py migrate
$ pipenv run ./manage.py createsuperuser
$ pipenv run ./manage.py runserver
```

---