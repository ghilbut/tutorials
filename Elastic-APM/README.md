Elastic-APM tutorial
====================

---

# 0. Prepare environment

## Run Database

```shell
$ cd $REPODIR/Elastic-APM/.local/db
$ docker-compose up -d
```

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