from django.urls import path
from . import web_views

urlpatterns = [
    path('',                       web_views.home,             name='home'),
    path('vehicles/',              web_views.vehicle_list,     name='vehicle_list'),
    path('vehicles/register/',     web_views.vehicle_register, name='vehicle_register'),
    path('vehicles/<str:plate>/',  web_views.vehicle_detail,   name='vehicle_detail'),
    path('plate-logs/',            web_views.plate_logs,       name='plate_logs'),
    path('violations/',            web_views.violations,       name='violations'),
    path('fines/',                 web_views.fines,            name='fines'),
    path('traffic/flow/',          web_views.traffic_flow,     name='traffic_flow'),
    path('traffic/congestion/',    web_views.congestion,       name='congestion'),
    path('traffic/map/',           web_views.traffic_map,      name='traffic_map'),
    path('traffic/route/',         web_views.route_guidance,   name='route_guidance'),
    path('emergency/',             web_views.emergency_control, name='emergency_control'),
]
