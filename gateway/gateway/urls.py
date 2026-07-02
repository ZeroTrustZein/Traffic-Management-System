from django.contrib import admin
from django.urls import path, include

urlpatterns = [
    path('admin/', admin.site.urls),
    path('',       include('gateway_app.api_urls')),
    path('',       include('gateway_app.web_urls')),
]
