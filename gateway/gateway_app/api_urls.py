from django.urls import re_path

from . import api_views


urlpatterns = [
    re_path(r'^api/(?P<api_path>.*)$', api_views.proxy_api, name='api_proxy'),
]
