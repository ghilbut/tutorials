from django.contrib.auth import (
    authenticate,
    login as django_login,
    logout as django_logout,
)
from django.contrib.auth.models import User
from django.http import (
    HttpResponse,
    HttpResponseNotAllowed,
    HttpResponseRedirect,
)
from django.shortcuts import (
    redirect,
    render,
)
from http import HTTPStatus

# Create your views here.


def index(request):
    context = { 'users': User.objects.all() }
    return render(request, 'index.html', context)


def index_error(request):
    context = { 'users': User.objects.all() }
    return render(request, 'index_error.html', context)


def health(request):
    return HttpResponse('OK')


def login(request):
    if request.user.is_authenticated:
        return HttpResponseRedirect('/')

    if request.method == 'POST':
        username = request.POST['username']
        password = request.POST['password']
        user = authenticate(request, username=username, password=password)
        if user is not None:
            django_login(request, user)
            return HttpResponseRedirect('/')
        context = { 'error': 'login failed' }
        return render(request, 'login.html', context, status=HTTPStatus.BAD_REQUEST)

    return render(request, 'login.html')


def logout(request):
    if request.method != 'POST':
        return HttpResponseNotAllowed()
    django_logout(request)
    return HttpResponseRedirect('/')


def exception(request):
    raise Exception('Tutorial Test')
